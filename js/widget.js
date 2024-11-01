function showAllTrails() {
    jQuery("#hidden_trails").show();
    jQuery("#show_all_trails").hide();
}
function hideExtraTrails() {
    jQuery("#hidden_trails").hide();
    jQuery("#show_all_trails").show();
}
function showTrailBlazing() {
    jQuery("#show_trail_blazing").hide();
    jQuery("#WordTrailsBlazingContainer").show();
}
function hideTrailBlazing() {
    jQuery("#show_trail_blazing").show();
    jQuery("#WordTrailsBlazingContainer").hide();
}
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
    if(jQuery("#remove_select > option:selected").val() == "null") {
        jQuery("#remove_submit").hide();
    } else {
        jQuery("#remove_submit").show();
    }
}

jQuery(window).load(function() {
    jQuery("#trail_blazing_form").submit(function() {
        switch(jQuery("#blaze_select > option:selected").attr('name')) {
            case "new_trail":
                var val = jQuery("#new_trail_options [name=new_trail_name]").attr('value');
                val = jQuery.trim(val);
                if (val == undefined || val == "") {
                    alert("Please enter a name for the new trail.");
                    return false;
                }
                break;
            case "null":
                return false;
        }
        return true;
    });
});
