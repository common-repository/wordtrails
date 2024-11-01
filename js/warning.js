function disableWarning(option) {
    jQuery("#disable_warning").parent().slideUp('normal');
    jQuery.post(ajax_url, {action:'wt_remove_warning', option:option}, (function(data) {
        checkNextWarning(jQuery("#disable_warning").parent());
    }));
}

function checkNextWarning(div) {
    jQuery.post(ajax_url, {action:'wt_check_warning'}, function(data) {
        if (data == "") {
            div.remove();
        } else {
            newdiv = jQuery(data);
            div.replaceWith(newdiv);
            newdiv.hide().slideDown('normal');
        }
    }, 'html');
}