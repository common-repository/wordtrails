<?php
$flashvars["auto_save_freq"] = "5";
$flashvars["changesets"] = "false";
$flashvars["lang_saved"] = "Draft Saved";
$flashvars["lang_save_changes"] = "Save Draft";
$flashvars["lang_saving"] = "Saving Draft";
?>
<script type="text/javascript" src="<?php echo WordTrailsGlobal::$urlpath; ?>js/autosave.js"></script>
<script type="text/javascript">
/* <![CDATA[ */
(function($) {
    $(document).ready(function() {
        $('.hide-if-no-js').show();
        $('.hide-if-js').hide();
    });
})(jQuery);
/* ]]> */
</script>