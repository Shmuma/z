function getObjRects(item) {
    var rects = new Array();
    rects['left'] = 0;
    rects['top'] = 0;
    
    var root_item = item;
    
    while (root_item) {
        rects['left'] = rects['left'] + root_item.offsetLeft;
        rects['top'] = rects['top'] + root_item.offsetTop;
        root_item = root_item.offsetParent;
    }

    rects['right'] = rects['left'] + item.width;
    rects['bottom'] = rects['top'] + item.height;
    rects['width'] = item.width;
    rects['height'] = item.height;

    return rects;
}

function floater_visible(e) {
    var floater = document.getElementById('floater');
    return (floater.style.visibility == "visible");
}

function floater_click(e, gid, sid) {
    // ie
    if (window.event) {
	e = event;
	
    	// middle or right button
	if (navigator.appName == "Microsoft Internet Explorer") {
        	if (e.button == 4 || e.button == 2) {
        	    return false;
        	}
	}
	else {
        	if (e.button == 1 || e.button == 2) {
        	    return false;
        	}
	}
    }
    // ff
    else {
    	// middle or right button
    	if (e.button == 1 || e.button == 2) {
    	    return false;
    	}
    }

    var floater = document.getElementById('floater');
    var screenitem = document.getElementById('screenitem_' + sid);
    
    sitem = getObjRects(screenitem);
    if (
        e.clientX >= (sitem['right'] - Math.round(sitem['width'] / 7)) &&
        e.clientX <= sitem['right'] &&
        (e.clientY + document.body.scrollTop) >= sitem['top'] &&
    	(e.clientY + document.body.scrollTop) <= (sitem['top'] + Math.round(sitem['height'] / 7))
       ) {
        if (graph_descs[gid]) {
            floater.style.top = e.clientY - 25 + document.body.scrollTop;
    	    floater.style.left = (e.clientX - 600 > 0 ? e.clientX - 600 : 0);
    	    floater.innerHTML = "<p>" + graph_descs[gid] + "</p><br/>";
    	    floater.style.visibility = "visible";
	    
            e.cancelBubble = true;
    	    if (e.stopPropagation) e.stopPropagation();
    	    return false;
    	}
    }
    else {
    	window.location = graph_links[gid];
    	return true;
    }
}

function floater_hide(e) {
    var floater = document.getElementById('floater');
    floater.style.visibility = "hidden";
}

function floater_check(e) {
    var floater = document.getElementById('floater');

    if (! floater_visible (e)) {
	   return true;
    }

    if (window.event) {
    	e = event;
    }
    
    h = 30;
    if (
        (e.clientX > floater.offsetLeft - h) &&
        (e.clientX < (floater.offsetWidth + floater.offsetLeft + h)) &&
    	(e.clientY + document.body.scrollTop > floater.offsetTop - h) &&
    	(e.clientY + document.body.scrollTop < (floater.offsetHeight + floater.offsetTop + h))
       ) {
    }
    else {
    	floater_hide(e);
    	return false;
    }
}
