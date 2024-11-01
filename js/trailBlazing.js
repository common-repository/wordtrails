function blazeSelect() {
    switch(jQuery("#blaze_select > option:selected").val()) {
	case "new_trail":
	    jQuery("#new_child_options").hide();
	    jQuery("#new_trail_options").show();
	    jQuery("#blaze_submit").show();
	    break;
	case "null":
	    jQuery("#new_child_options").hide();
	    jQuery("#new_trail_options").hide();
	    jQuery("#blaze_submit").hide();
	    break;
	default:
	    jQuery("#new_child_options").show();
	    jQuery("#new_trail_options").hide();
	    jQuery("#blaze_submit").show();
    }
}

function removeSelect() {
    switch(jQuery("#remove_select > option:selected").val()) {
	case "null":
	    jQuery("#remove_submit").hide();
	    break;
	default:
	    jQuery("#remove_submit").show();
	    break;
    }
}

jQuery(document).ready(function() {
    jQuery("form:has(#blaze_select)").submit(function() {
	if(jQuery("#blaze_select > option:selected").attr('name') == "new_trail") {
		var val = jQuery("#new_trail_options [name=new_trail_name]").attr('value');
		val = jQuery.trim(val);
		if (val == undefined || val == "") {
		    alert("Please enter a name for the new trail.");
		    return false;
		}
	}
	return true;
    });
});