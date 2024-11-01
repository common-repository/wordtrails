var save_shown = false;
(function($) {
    $(document).ready(function() {
        if (!$("#commit").hasClass("hide-if-js")) save_shown = true;
    });
})(jQuery);
function autosaved() {
    if (save_shown) return true;
    jQuery("#commit").slideDown('normal', function() {
        save_shown = true;
    });
}