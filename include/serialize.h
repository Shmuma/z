#ifndef ZABBIX_SERIALIZE_H
#define ZABBIX_SERIALIZE_H

extern size_t DB_ITEM_OFFSETS[CHARS_LEN_MAX];

size_t dbitem_size(DB_ITEM *item, int update_len);
void dbitem_serialize(DB_ITEM *item, size_t item_len);
void dbitem_unserialize(char *str, DB_ITEM *item);

#endif // ZABBIX_SERIALIZE_H
