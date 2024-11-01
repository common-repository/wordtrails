var count = 0;
jQuery(document).ready(function() {
    jQuery('.flash-change').each(function() {
	$self = jQuery(this);
	$self.data("bg", {"backgroundColor":$self.css("backgroundColor")});
	$self.animate(
	    { backgroundColor: '#ffffe0' },
	    300
	).animate(
	    { backgroundColor: '#fffbcc' },
	    300
	).animate(
	    { backgroundColor: '#ffffe0' },
	    300
	).animate(
	    $self.data("bg"),
	    300
	);
    });
    checkNewTrailsPage();
    var p, e = jQuery('#help_rss div.inside:visible').find('.widget-loading');
    if ( e.length ) {
	p = e.parent();
	p.load(ajax_url,{action:"wt_help_rss_display"}, function() {
	    p.hide().slideDown('normal', function(){
		jQuery(this).css('display', '');
	    });
	});
    }
});

function checkNewTrailsPage() {
    var val = jQuery("#new_trails_page_name").attr("value");
    if (val == "" || val == undefined || (val.length <= 2 && isNaN(parseInt(val)))) {
	jQuery("#new_trails_submit").attr({disabled: "disabled", value: "Enter a Name"});
	return false;
    }
    var c = ++count;
    jQuery("#new_trails_submit").attr({disabled: "disabled", value: "Checking..."});
    jQuery.post(ajax_url, {action: "wt_check_new_trails_page", val: val, check_count: c}, null,"script");
    return false;
}

function checkNewTrailsPageResults(exists, c, val) {
    if (c == count) {
	jQuery("#new_trails_submit").removeAttr("disabled");
	if (exists) {
	    jQuery("#new_trails_submit").attr('value', "Take Over Existing Page");
	} else {
	    if (isNaN(parseInt(val))) {
	        jQuery("#new_trails_submit").attr('value', "Create New Page to display Trails");
	    } else {
	        jQuery("#new_trails_submit").attr({disabled: "disabled", value: "Page Not Found"});
	    }
	}
    }
}

function setDefault(parent, child, jump) {
    jQuery.post(ajax_url+jump, {action:"wt_set_default_child",parent:parent,child:child},null, "script");
    return false;
}
function confirmDefaultChild(child) {
    jQuery("#children_del_rel :radio").removeAttr("checked").blur().filter("[name=child_radio_" + child+"]").attr("checked", "checked");
    window.location.href = window.location.href;
}
jQuery.fn.wait = function(time, type) {
    time = time || 1000;
    type = type || "fx";
    return this.queue(type, function() {
	var self = this;
	setTimeout(function() {
	    jQuery(self).dequeue();
	}, time);
    });
};

function GatherAndPushTrails(formID) {
    var form = jQuery("#"+formID);
    var trails = form.find("tbody :checkbox:enabled:checked");
    if (trails.length == 0) return;
    var hashes = new Array();
    for (var i=0; i<trails.length; i++) {
	hashes.push(trails.eq(i).attr("name"));
    }
    var sel_rows = form.find("#tr_"+hashes.join(", #tr_"));
    form.find("thead :checkbox").removeAttr("checked");
    sel_rows.find(":checkbox").removeAttr("checked").attr("disabled", "disabled").end().find(".sync_date").each(function(index, domel) {
	domel=jQuery(domel);
	//alert(domel.html());
	domel.data("oldhtml", domel.html());
    }).html("<i>Starting...</i>");

    if (hashes.length == form.find("tbody tr").length) {
	sel_rows.find(".sync_date").html("<i>Preparing Data...</i>");
	jQuery.post(ajax_url, {action:"wt_trailmeme_push"}, null, "script");
    } else {
	for (var i=0; i<hashes.length; i++) {
	    sel_rows.find("#"+hashes[i]+ " .sync_date").html("<i>Preparing Data...</i>");
	    jQuery.post(ajax_url, {action:"wt_trailmeme_push", hash:hashes[i]},null, "script");
	}
    }

}
function pushError(error, hash) {
    if (hash) {
	status = jQuery("#tr_"+hash+" .sync_date");
	status.html("<i><em>"+error+"</em></i>");
	jQuery("#tr_"+hash+" :checkbox").removeAttr("disabled").removeAttr("checked");
	if (status.data("oldhtml") != undefined) {
	    status.wait(5000).fadeOut("slow", function() {
		status = jQuery("#tr_"+hash+" .sync_date");
		status.html(status.data("oldhtml")).fadeIn("slow");
	    });
	}
    } else {
	alert(error);
    }
}

function pushFinished(hash, result, time) {
    result = unescape(result);
    //alert(result);
    jQuery("#tr_"+hash+" :checkbox").removeAttr("disabled").removeAttr("checked");
    if (result == "Data successfully queued for import.") {
	jQuery("#tr_"+hash+" .sync_date").html("<i>Finished!</i>").wait(2000).fadeOut("slow", function() {jQuery("#tr_"+hash+" .sync_date").html("Last Sent: "+time).fadeIn("slow");});
    } else {
	jQuery("#tr_"+hash+" .sync_date").html("<i>TrailMeme Error: " + result);//.wait(2000).fadeOut("slow", function() {jQuery("#"+hash+" [@name=sync_date]").html(time).fadeIn("slow");});
    }
}

jQuery(document).ready(function() {
    jQuery(".trail-delete").click(function() {
	$button = jQuery(this);
	$button.attr("disabled", "disabled");
	cls = "";
	if ($button.hasClass("button-secondary")) {
	    cls = " button-secondary";
	} else if ($button.hasClass("button-primary")) {
	    cls = " button-primary";
	}
	ins = jQuery("<div style=\"clear:both;\"><strong>Are you sure?</strong><br \><input type=\"button\" class=\"yes"+cls+"\" value=\"Yes\" alt=\"Delete\" /> <input type=\"button\" class=\"no"+cls+"\" value=\"No\" alt=\"Cancel\" /><div>").hide();
	ins.find(".yes").click(function() {
	    $yes = jQuery(this);
	    prnt = $yes.parent();
	    while(prnt.is(":not('form')")) {
		prnt = prnt.parent();
	    }
	    prnt.children(":first").val("delete_node");
	    prnt.submit();
	});
	ins.find(".no").click(function() {
	    $no = jQuery(this);
	    prnt = $no.parent();
	    while(prnt.is(":not('td')")) {
		prnt = prnt.parent();
	    }
	    prnt.find(".trail-delete").attr("disabled", "");
	    jQuery(this).parent().slideUp("normal", function() {jQuery(this).remove();});
	});
	$button.parent().children(":last").after(ins);
	ins.slideDown("normal");

    });
    jQuery("#TrailViewer").wrap("<div></div>").parent().addClass("rounded");
    if (jQuery("#new_trails_page_name")) {
	$inpt = jQuery("#new_trails_page_name");
	var cssObj = {color: $inpt.css("color"), fontStyle: $inpt.css("fontStyle")};
	$inpt.data("normal", cssObj);
	var cssObjDefault = {color: "#999", fontStyle: "italic"};
	$inpt.data("default", cssObjDefault);
	$inpt.css($inpt.data("default"));
	$inpt.focus(function() {
	    $txt = jQuery(this);
	    $txt.css($txt.data("normal"));
	    if ($txt.val() == "Trails") {
		$txt.select();
		checkNewTrailsPage();
	    }
	});
	$inpt.blur(function() {
	    $txt = jQuery(this);
	    if ($txt.val() == "" || $txt.val() == "Trails") {
		$txt.css($txt.data("default"));
		$txt.val("Trails");
	    }
	})
    }
});
