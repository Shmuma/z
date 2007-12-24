/* 
** ZABBIX
** Copyright (C) 2000-2005 SIA Zabbix
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**/

#include "common.h"
#include "sysinc.h"
#include "log.h"
#include "active.h"
#include "activebuf.h"


extern int      CONFIG_ACTIVE_BUF_SIZE_MB;
extern char     *CONFIG_ACTIVE_BUF_FILE;

static const unsigned int buffer_signature = 0x3ABBB0FF;

/* 4 bytes for timestamp + 32 bytes for value (string) */
static const unsigned int entry_size_estimation = 36;

/* This defines after which amount of items interval of added items in buffer after which buffer is flushed */
#define FLUSH_EVERY_ITEMS 10
static int flush_counter;



typedef struct {
    char *key;
    int max_items, index, count, refresh;
    unsigned short* sizes;

    /* Actual entry marks. Remains untouched until there are entries in buffer/ */
    /* begin offset of this entry's buffer */
    size_t beg_offset;

    /* offset of next entry */
    size_t max_offset;
} buffer_check_entry_t;


typedef struct  {
    int active;
    FILE* file;
    size_t header_size;

    unsigned int size;
    buffer_check_entry_t* entries;
    unsigned int items;
    ZBX_ACTIVE_METRIC* new_entries;
} buffer_state_t;



buffer_state_t buffer = { .active = 0 };


static int read_initial_buffer ();
static int create_initial_buffer ();
static int apply_new_entries ();
static void deactivate_buffer ();
static void free_entries ();
static void flush_buffer ();




void init_active_buffer ()
{
    struct stat statbuf;

    if (!CONFIG_ACTIVE_BUF_FILE || !CONFIG_ACTIVE_BUF_SIZE_MB)
        return;

    zabbix_log (LOG_LEVEL_DEBUG, "In init_active_buffer. Buffer file %s, size %d Mb", CONFIG_ACTIVE_BUF_FILE, CONFIG_ACTIVE_BUF_SIZE_MB);

    /* is buffer file already exists? */
    if (stat (CONFIG_ACTIVE_BUF_FILE, &statbuf) == 0) {
        /* trying to open buffer file */
        buffer.file = fopen (CONFIG_ACTIVE_BUF_FILE, "r+b");

        if (!buffer.file) {
            zabbix_log (LOG_LEVEL_WARNING, "Active checks buffer file cannot be opened. Error: '%s'. Buffer disabled.", strerror (errno));
            return;
        }
    }
    else {
        /* trying to create new buffer file */
        buffer.file = fopen (CONFIG_ACTIVE_BUF_FILE, "w+b");

        if (!buffer.file) {
            zabbix_log (LOG_LEVEL_WARNING, "Active checks buffer file cannot be created. Error: '%s'. Buffer disabled.", strerror (errno));
            return;
        }
    }

    buffer.active = 1;
    buffer.size  = 0;
    buffer.items = 0;
    buffer.entries = NULL;
    buffer.new_entries = NULL;

    if (!read_initial_buffer ())
        if (!create_initial_buffer ()) {
            /* Something really bad with our system (disk full?). It
               is really better to live without buffer. */
            deactivate_buffer ();
        }
}



void free_active_buffer ()
{
    zabbix_log (LOG_LEVEL_DEBUG, "in free_active_buffer ()");

    if (!buffer.active)
        return;

    if (!buffer.file)
        return;                 /* this is really strange case */

    fclose (buffer.file);
    deactivate_buffer ();
}



static void flush_buffer ()
{
    int i;

    /* flush buffer state */
    zabbix_log (LOG_LEVEL_DEBUG, "Flushing metainformation in active buffer for %d items", buffer.size);
    fseek (buffer.file, sizeof (buffer_signature), SEEK_SET);
    fwrite (&buffer.size, sizeof (unsigned int),  1, buffer.file);

    for (i = 0; i < buffer.size; i++) {
        fwrite (buffer.entries[i].key, strlen (buffer.entries[i].key)+1, 1, buffer.file);
        fwrite (&buffer.entries[i].refresh, sizeof (int), 1, buffer.file);
        fwrite (&buffer.entries[i].beg_offset, sizeof (size_t), 1, buffer.file);
        fwrite (&buffer.entries[i].max_offset, sizeof (size_t), 1, buffer.file);
        fwrite (&buffer.entries[i].max_items,  sizeof (int), 1, buffer.file);
        fwrite (buffer.entries[i].sizes,  sizeof (unsigned short), buffer.entries[i].max_items, buffer.file);
    }

    fflush (buffer.file);
    flush_counter = 0;
}




/* Rebuild structure of buffers. Actual modifications occur only after buffers are freed. */
void update_active_buffer (ZBX_ACTIVE_METRIC* active)
{
    if (!buffer.active)
        return;

    zabbix_log (LOG_LEVEL_DEBUG, "update_active_buffer");
    
    buffer.new_entries = active;
    
    if (!buffer.items)
        if (!apply_new_entries ())
            deactivate_buffer ();
}


static int read_initial_buffer ()
{
    unsigned long sig;
    unsigned int i, j, len;
    int index;
    char c;
    char buf[1024];

    zabbix_log (LOG_LEVEL_DEBUG, "read_initial_buffer");

    /* check signature */
    if (!fread (&sig, sizeof (sig), 1, buffer.file))
        return 0;

    if (sig != buffer_signature)
        return 0;

    if (fread (&buffer.size, sizeof (unsigned int), 1, buffer.file) != 1)
        return 0;

    if (!buffer.size)           /* read finished, there are no items in buffer */
        return 1;

    zabbix_log (LOG_LEVEL_DEBUG, "Reading %d items of history from buffer", buffer.size);

    buffer.entries = (buffer_check_entry_t*)calloc (buffer.size, sizeof (buffer_check_entry_t));

    for (i = 0; i < buffer.size; i++) {
        buffer.entries[i].key = NULL;
        len = 0;
        while ((c = fgetc (buffer.file))) {
            buf[len++] = c;
            if (len == 1023) {
                buf[len] = 0;
                buffer.entries[i].key = zbx_strdcat (buffer.entries[i].key, buf);
                len = 0;
            }
        }

        if (len) {
            buf[len] = 0;
            buffer.entries[i].key = zbx_strdcat (buffer.entries[i].key, buf);
        }

        fread (&buffer.entries[i].refresh, sizeof (int), 1, buffer.file);
        fread (&buffer.entries[i].beg_offset, sizeof (size_t), 1, buffer.file);
        fread (&buffer.entries[i].max_offset, sizeof (size_t), 1, buffer.file);
        fread (&buffer.entries[i].max_items,  sizeof (int), 1, buffer.file);

        buffer.entries[i].sizes = (unsigned short*)malloc (buffer.entries[i].max_items * sizeof (unsigned short));
        fread (buffer.entries[i].sizes, sizeof (unsigned short), buffer.entries[i].max_items, buffer.file);

        /* find index (hm-hm) and item's count */
        index = -1;
        buffer.entries[i].count = 0;
        buffer.entries[i].index = 0;

        for (j = 0; j < buffer.entries[i].max_items; j++) {
            if (!buffer.entries[i].sizes[j]) {
                if (index < 0)
                    index = j;
            }
            else
                buffer.entries[i].count++;
        }

        if (index >= 0)
            buffer.entries[i].index = index;

        zabbix_log (LOG_LEVEL_DEBUG, "Read item history. Key %s, refresh %d, len: %lu, items: %d, index: %d, count: %d", buffer.entries[i].key, 
                    buffer.entries[i].refresh, buffer.entries[i].max_offset - buffer.entries[i].beg_offset, buffer.entries[i].max_items,
                    buffer.entries[i].index, buffer.entries[i].count);
    }

    return 1;
}


static int create_initial_buffer ()
{
    fseek (buffer.file, 0, SEEK_SET);
    if (!fwrite (&buffer_signature, sizeof (buffer_signature), 1, buffer.file))
        return 0;

    return 1;
}


static int apply_new_entries ()
{
    int count, i;
    unsigned long s = 0, t;
    size_t ofs;

    zabbix_log (LOG_LEVEL_DEBUG, "apply_active_buffer");

    free_entries ();

    if (!buffer.new_entries)
        return 1;

    /* get amount of active checks */
    count = i = 0; 
    while (buffer.new_entries[i++].key)
        count++;

    zabbix_log (LOG_LEVEL_DEBUG, "apply_active_buffer: count of new checks %d", count);

    buffer.entries = (buffer_check_entry_t*)malloc (count*sizeof (buffer_check_entry_t));

    if (!buffer.entries)
        return 0;

    buffer.header_size = sizeof (buffer_signature) + sizeof (unsigned int);
    buffer.size = count;
    zabbix_log (LOG_LEVEL_DEBUG, "apply_active_buffer: memory for new entries allocated");

    if (count) {
        for (i = 0; i < count; i++) {
            buffer.entries[i].key = strdup (buffer.new_entries[i].key);

            if (buffer.new_entries[i].refresh)
                s += (entry_size_estimation * 60UL) / buffer.new_entries[i].refresh;
            else
                s += (entry_size_estimation * 60UL) / 1;
        }

        /* here we have estimation of our buffer space in minutes */
        t = (CONFIG_ACTIVE_BUF_SIZE_MB * 1024UL*1024UL) / s;

        zabbix_log (LOG_LEVEL_DEBUG, "apply_active_buffer: New buffer's estimation time to live is %lu minutes", t);

        ofs = 0;
        s = 0;

        for (i = 0; i < count; i++) {
            buffer.header_size += strlen (buffer.entries[i].key) + 1 + sizeof (size_t)*2;
            buffer.entries[i].beg_offset = ofs;

            if (buffer.new_entries[i].refresh)
                ofs += t*(entry_size_estimation * 60UL) / buffer.new_entries[i].refresh;
            else
                ofs += t*(entry_size_estimation * 60UL) / 1;

            buffer.entries[i].max_offset = ofs;
            buffer.entries[i].index = 0;
            buffer.entries[i].count = 0;
            buffer.entries[i].refresh = buffer.new_entries[i].refresh;
            buffer.entries[i].max_items = (buffer.entries[i].max_offset-buffer.entries[i].beg_offset) / 6;
            buffer.entries[i].sizes = (unsigned short*)calloc (sizeof (unsigned short), buffer.entries[i].max_items);
            buffer.header_size += buffer.entries[i].max_items*sizeof (unsigned short);

            zabbix_log (LOG_LEVEL_DEBUG, "Item %s: refr: %d, max_items = %d", buffer.entries[i].key, 
                        buffer.new_entries[i].refresh, buffer.entries[i].max_items);
            s += buffer.entries[i].max_offset-buffer.entries[i].beg_offset;
        }

        zabbix_log (LOG_LEVEL_DEBUG, "Total size of buffers %d", s);
    }

    /* all finished ok */
    buffer.new_entries = NULL;
    flush_buffer ();

    return 1;
}



static void deactivate_buffer ()
{
    fclose (buffer.file);
    buffer.active = 0;
}



static void free_entries ()
{
    int i;

    if (!buffer.size)
        return;

    for (i = 0; i < buffer.size; i++) {
        free (buffer.entries[i].key);
        free (buffer.entries[i].sizes);
    }

    free (buffer.entries);
    buffer.entries = NULL;
    buffer.size = 0;
}


void store_in_active_buffer (const char* key, const char* value)
{
    int i, len = strlen (value), fill, p;
    time_t ts = time (NULL);
    buffer_check_entry_t* e;
    size_t ofs;
    unsigned char zero = 0;

    if (!buffer.active)
        return;

    zabbix_log (LOG_LEVEL_DEBUG, "store_in_active_buffer ()");

    for (i = 0; i < buffer.size; i++) {
        if (strcmp (buffer.entries[i].key, key))
            continue;

        e = buffer.entries+i;
        zabbix_log (LOG_LEVEL_DEBUG, "Item found. It's %d-th item. Count: %d, It has index %d, beg: %lu, end: %lu", i, e->count, e->index, e->beg_offset, e->max_offset);

        /* find space to store our value */
        ofs = e->beg_offset;
        for (i = 0; i < e->index; i++)
            ofs += e->sizes[i];

        if (ofs + sizeof (ts) + len + 1 > e->max_offset) {
            zabbix_log (LOG_LEVEL_DEBUG, "Wrap detected!");
            e->index = 0;
            ofs = e->beg_offset;
        }

        fill = 0;

        if (e->sizes[e->index] > 0) {
            /* if there is another item on our way, wipe it out */
            p = e->index;

            while (fill < sizeof (ts) + len + 1) {
                fill += e->sizes[p];
                e->sizes[p] = 0;
                e->count--;
                buffer.items--;
                p++;
            }
        }

        e->sizes[e->index] = sizeof (ts) + len + 1;
        fseek (buffer.file, ofs+buffer.header_size, SEEK_SET);
        fwrite (&ts, sizeof (ts), 1, buffer.file);
        fwrite (value, len+1, 1, buffer.file);

        if (fill > 0) {
            fill -= e->sizes[e->index];
            fwrite (&zero, sizeof (unsigned char), fill, buffer.file);
            e->sizes[e->index] += fill;
        }

        e->index++;
        e->count++;
        e->index %= e->max_items;
        zabbix_log (LOG_LEVEL_DEBUG, "New item properties: index %d, count: %d, beg: %lu, end: %lu", e->index, e->count, e->beg_offset, e->max_offset);
        buffer.items++;
        break;
    }
    
    if (++flush_counter == FLUSH_EVERY_ITEMS)
        flush_buffer ();
}


int  active_buffer_is_empty ()
{
    return buffer.active ? !buffer.items : 1;
}


active_buffer_item_t* take_active_buffer_item ()
{
    active_buffer_item_t* item = NULL;
    buffer_check_entry_t* e;
    int i, j, index;
    size_t ofs;

    if (active_buffer_is_empty ())
        return NULL;

    zabbix_log (LOG_LEVEL_DEBUG, "take_active_buffer_item ()");
    zabbix_log (LOG_LEVEL_DEBUG, "Buffer size %d, items %d", buffer.size, buffer.items);
    e = buffer.entries;

    for (i = 0; i < buffer.size; i++, e++) {
        if (!e->count)
            continue;

        item = (active_buffer_item_t*)malloc (sizeof (active_buffer_item_t));

        if (!item) {
            zabbix_log (LOG_LEVEL_WARNING, "Malloc failed");
            return NULL;
        }

        zabbix_log (LOG_LEVEL_DEBUG, "Entry %s, size %d", e->key, e->count);
        
        item->key = strdup (e->key);
        item->size = e->count;
        item->refresh = e->refresh;
        item->ts = (unsigned long*)malloc (sizeof (unsigned long) * e->count);
        item->values = (char**)malloc (sizeof (char*) * e->count);

        index = 0;
        ofs = e->beg_offset + buffer.header_size;
        for (j = 0; j < e->max_items; j++)
            if (e->sizes[j]) {
                unsigned long ts;
                char* val = (char*)malloc (e->sizes[j]-sizeof (ts));

                /* we should add more debugging about this code */
                fseek (buffer.file, ofs, SEEK_SET);
                fread (&ts, sizeof (ts), 1, buffer.file);
                fread (val, e->sizes[j]-sizeof (ts), 1, buffer.file);

                item->ts[j] = ts;
                item->values[j] = val;

                ofs += e->sizes[j];
            }

        /* clean items after take */
        e->count = e->index = 0;
        break;
    }

    zabbix_log (LOG_LEVEL_DEBUG, "take_active_buffer_items () exit");

    return item;
}



void free_active_buffer_item (active_buffer_item_t* item)
{
    int i;

    if (!buffer.active)
        return;

    for (i = 0; i < item->size; i++) 
        free (item->values[i]);
    free (item->values);
    free (item->ts);
    free (item);
}


active_buffer_checks_t* get_buffer_checks_list ()
{
    active_buffer_checks_t* items;
    int i;

    if (!buffer.active)
        return NULL;

    zabbix_log (LOG_LEVEL_DEBUG, "get_buffer_checks_list ()");

    items = (active_buffer_checks_t*)malloc (sizeof (active_buffer_checks_t));

    if (!items) {
        zabbix_log (LOG_LEVEL_WARNING, "Memory allocation error");
        return NULL;
    }

    items->size = buffer.size;
    items->keys = (char**)malloc (sizeof (char*) * buffer.size);

    if (!items->keys) {
        zabbix_log (LOG_LEVEL_WARNING, "Memory allocation error");
        free (items);
        return NULL;
    }

    items->refreshes = (int*)malloc (sizeof (int) * buffer.size);

    if (!items->refreshes) {
        zabbix_log (LOG_LEVEL_WARNING, "Memory allocation error");
        free (items->keys);
        free (items);
        return NULL;
    }

    for (i = 0; i < buffer.size; i++) {
        items->keys[i] = strdup (buffer.entries[i].key);
        items->refreshes[i] = buffer.entries[i].refresh;
    }

    zabbix_log (LOG_LEVEL_DEBUG, "get_buffer_checks_list () exit");

    return items;
}


void free_buffer_checks_list (active_buffer_checks_t* checks)
{
    if (!checks)
        return;

    if (checks->keys)
        free (checks->keys);
    if (checks->refreshes)
        free (checks->refreshes);
    free (checks);
}
