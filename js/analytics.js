var $loadQueue = new Array(), $loading = false, $prev = false, $totals = null, passParams = new Array("wt_trail_sort", "trails_per_page", "trails_paged", "wt_sid_sort", "sessions_per_page", "sessions_paged");
jQuery(document).ready(function() {
    loadTotals();
    autoload();
    jQuery.fn.forceShow = function() {
	retme = this.each(function() {
	    $link = jQuery(this);
	    $link.data("target").show();
            if ($link.data("target").children().eq(0).hasClass("empty"))
                queue($link.data("target").children().eq(0));
	    $link.parent().parent().children().css({"borderBottom": "none"});
	});
	storeExpanded();
	return retme;
    };
    jQuery.fn.forceHide = function() {
	retme = this.each(function() {
	    $link = jQuery(this);
	    $link.data("target").hide();
	    if ($link.data("target").children().eq(0).hasClass("waiting"))
                unqueue($link.data("target").children().eq(0));
	    $link.parent().parent().children().css($link.data("brdr"));
	});
	storeExpanded();
	return retme;
    };
    jQuery(".expand-link").each(function() {
	$link = jQuery(this);
	var target = $link.parent().parent().children(":last");
	var cssObj = {borderBottomWidth : target.css("borderBottomWidth"), borderBottomStyle : target.css("borderBottomStyle"), borderBottomColor : target.css("borderBottomColor")};
	$link.data("brdr", cssObj);
	$link.data("target", $link.parent().parent().next());
	if ($link.data("target").is(":visible")) $link.parent().parent().children().css({borderBottom: "none"});
    }).click(function() {
	$link = jQuery(this);
	$link.data("target").toggle();
        if ($link.data("target").is(":visible")) {
            if ($link.data("target").children().eq(0).hasClass("empty"))
                queue($link.data("target").children().eq(0));
	} else {
	    if ($link.data("target").children().eq(0).hasClass("waiting"))
                unqueue($link.data("target").children().eq(0));
	}
	$link.parent().parent().children().css(($link.data("target").is(":visible") ? {borderBottom: "none"} : $link.data("brdr")));
	storeExpanded();
    });
});

function urlParam(name) {
    var results = new RegExp('[\\?&]' + name + '=([^&#]*)').exec(window.location.href);
    if (!results) return 0;
    return results[1] || 0;
}
function appendParams(args) {
    if (typeof args != "object") args = {};
    jQuery.each(passParams, function(index, ele) {
	if (args[ele]) return true;
	var $val = urlParam(ele);
	if ($val) args[ele] = $val;
	return true;
    });
    return args;
}

function autoload($null, args) {
    loadnum = 1;
    if (args) {
	if (args.autoload) loadnum = args.autoload;
    } else {
	args = {};
    }
    jQuery(".autoload"+loadnum).each(function (index) {
        queue(this);
    });
    args.autoload = loadnum + 1;
    if ($null == null) $null = jQuery("<div/>");
    $q = {func:autoload, ele:$null, args:args};
    if (loadnum < 4) $loadQueue.push($q);
    if ($totals) nextInQueue();
}

function unqueue(ele) {
    if ($loadQueue.length) {
	$ele = jQuery(ele);
	jQuery.each($loadQueue, function(index) {
	    if (this.ele) {
		if (this.ele == $ele) {
		    $loadQueue.splice(index,1);
		    return false;
		}
	    }
	    return true;
	});
    }
}

function queue(ele) {
    if (ajax_url == null || ajax_url == undefined) return false;
    $ele = jQuery(ele);
    if ($ele.parent().is(":hidden")) $ele.parent().toggle();
    $q = {func: jQuery.noop, args: null, ele: $ele};
    id = $ele.attr("id");
    //#high_level_trail_{trail-hash}
    //#mid_level_trail_{trail-hash}_sid_{sid}
    //#detail_trail_{trail-hash}_sid_{sid}
    args = {};
    spl = id.split("_");
    if (id.indexOf("high_level_") != -1) {
        $q.func = preloadHighLevel;
        if (spl[2] == "trail") args.hash = spl[3];
        $ele.find("#status").html("Waiting...");
    } else if (id.indexOf("mid_level_") != -1) {
        $q.func = loadMidLevel;
        if (spl[2] == "trail") args.hash = spl[3];
        $ele.html("Waiting ...");
    } else if (id.indexOf("detail_") != -1) {
        $q.func = loadDetails;
        if (spl[1] == "trail") args.hash = spl[2];
        if (spl[3] == "sid") args.sid = spl[4];
        $ele.html("Waiting ...");
    } else {
        //wtf is this? pull up! pull up!
        return false;
    }
    if (args.hash != null && args.hash != undefined) {
        $q.args = args;
        $loadQueue.push($q);
        $ele.addClass("waiting");
        if (!$loading) nextInQueue();
        return true;
    } else {
        return false;
    }
}

function nextInQueue() {
    if ($loadQueue.length == 0) {
        $loading = false;
        return;
    }
    $loading = true;
    next = $loadQueue.shift();
    next.ele.removeClass("waiting");
    next.ele.addClass("loading");
    if (!next.args.autoload) watchTimeout(next);
    next.func(next.ele, next.args);
}

function preloadHighLevel($ele, args) {
    vars = {action: "wt_pre_load_high_level", hash: args.hash};
    if ($totals[args.hash]) vars.total = $totals[args.hash];
    $status = "Loading...";
    if (args.percent) $status += " " + args.percent + "%";
    else if (args.percent == 0) $status += " 0%";
    $ele.find("#status").html($status);
    jQuery.post(ajax_url, vars, function(rt, ts, hr) {preLoadProgress(rt, $ele, args, preloadHighLevel, loadHighLevel);}, "json");
}

function preLoadProgress(response, $ele, args, prefunc, nextfunc) {
    if (response) args.percent = response.percent;
    if (args.percent < 100) {
	prefunc($ele, args);
    } else {
	nextfunc($ele, args);
    }
}

function loadHighLevel($ele, args) {
    vars = {action: "wt_load_high_level", hash: args.hash};
    vars = appendParams(vars);
    $status = "Crunching...";
    $ele.find("#status").html($status);
    jQuery.post(ajax_url, vars, (function(rt, ts, hr) { loadFinished(rt, $ele); }), "html");
}

function loadMidLevel($ele, args) {
    vars = {action: "wt_load_mid_level", hash: args.hash};
    vars = appendParams(vars);
    $status = "Loading...";
    if (args.retry)
	$status += " Timedout while crunching. Retry: " + args.retry;
    $ele.html($status);
    jQuery.post(ajax_url, vars, (function(rt, ts, hr) { loadFinished(rt, $ele); }), "html");
}

function loadDetails($ele, args) {
    vars = {action: "wt_load_details", hash: args.hash, sid: args.sid};
    vars = appendParams(vars);
    $status = "Loading...";
    if (args.retry)
	$status += " Timedout while crunching. Retry: " + args.retry;
    $ele.html($status);
    jQuery.post(ajax_url, vars, (function(rt, ts, hr) { loadFinished(rt, $ele); }), "html");
}

function watchTimeout(fromQueue) {
    if (fromQueue.func == preloadHighLevel) return;
    timer = window.setTimeout(timedOut, 30000, fromQueue);
    fromQueue.ele.data("timer", timer);
}

function timedOut(fromQueue) {
    if (fromQueue.args.retry) {
	fromQueue.args.retry = fromQueue.args.retry + 1;
    } else {
	fromQueue.args.retry = 1;
    }
    watchTimeout(fromQueue);
    fromQueue.func(fromQueue.ele, fromQueue.args);
}

function loadFinished($html, $ele) {
    $ele.removeClass("loading empty");
    window.clearTimeout($ele.data("timer"));
    $ele.html($html);
    $ele.find(".expand-link").each(function() {
	$link = jQuery(this);
	var target = $link.parent().parent().children(":last");
	var cssObj = {borderBottomWidth : target.css("borderBottomWidth"), borderBottomStyle : target.css("borderBottomStyle"), borderBottomColor : target.css("borderBottomColor")};
	$link.data("brdr", cssObj);
	$link.data("target", $link.parent().parent().next());
	if ($link.data("target").is(":visible")) $link.parent().parent().children().css({borderBottom: "none"});
    }).click(function() {
	$link = jQuery(this);
	$link.data("target").toggle();
        if ($link.data("target").is(":visible"))
            if ($link.data("target").children().eq(0).hasClass("empty"))
                queue($link.data("target").children().eq(0));
	$link.parent().parent().children().css(($link.data("target").is(":visible") ? {borderBottom: "none"} : $link.data("brdr")));
	storeExpanded();
    });
    
    $ele.find(".tipthis").each(function() {
        $node = jQuery(this);
        $node.tooltip(tooltipObj);
    });
    nextInQueue();
}

function storeExpanded() {
    var expanded = new Array();
    var collapsed = new Array();
    jQuery(".expand-row").each(function(i) {
	if (!jQuery(this).is(":hidden")) {
	    expanded.push(jQuery(this).attr("id"));
	} else {
	    collapsed.push(jQuery(this).attr("id"));
	}
    });
    jQuery.post(ajax_url, {action:"wt_store_expanded_analytics",expanded: expanded.join(","),collapsed:collapsed.join(",")},null,"script");
    return true;
}

function loadTotals() {
    $loading = true;
    jQuery.post(ajax_url, {action:"wt_get_trail_session_counts"}, processTotals, "json");
}
function processTotals(totals) {
    $totals = totals;
    $loding = false;
    nextInQueue();
}