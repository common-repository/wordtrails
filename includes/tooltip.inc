<?php
function wt_tooltip_head() { ?>
    <link rel="stylesheet" type="text/css" href="<?php echo WordTrailsGlobal::$urlpath; ?>css/tooltip.css" />
    <style type="text/css">
	<!--
	#tooltip.pretty .wrap {
	    background: url('<?php echo WordTrailsGlobal::$urlpath; ?>imgs/tooltip/tooltip_fill.png') repeat-y;
	}
	#tooltip.pretty .tip-top {
	    background: url('<?php echo WordTrailsGlobal::$urlpath; ?>imgs/tooltip/tooltip_top.png') no-repeat;
	}
	#tooltip.pretty .tip-bottom {
	    background: url('<?php echo WordTrailsGlobal::$urlpath; ?>imgs/tooltip/tooltip_bottom.png') no-repeat;
	}
	#tooltip.pretty.viewport-bottom .tip-top {
	    background: url('<?php echo WordTrailsGlobal::$urlpath; ?>imgs/tooltip/tooltip_vpb_top.png') no-repeat;
	}
	#tooltip.pretty.viewport-bottom .tip-bottom {
	    background: url('<?php echo WordTrailsGlobal::$urlpath; ?>imgs/tooltip/tooltip_vpb_bottom.png') no-repeat;
	}<?php
	if (!is_null(WordTrailsGlobal::get_option(WordTrailsGlobal::TRAILS_PAGE_OPTION))) {
	    if (is_page(WordTrailsGlobal::get_option(WordTrailsGlobal::TRAILS_PAGE_OPTION))) { ?>

	#WordTrails .walks {
	    background-image:url('<?php echo WordTrailsGlobal::$urlpath; ?>imgs/walks.png');
	}
	#WordTrails .start {
	    background-image:url('<?php echo WordTrailsGlobal::$urlpath; ?>imgs/start.png');
	}<?php
	    }
	} ?>

    -->
    </style>
    <script type="text/javascript">
    <!--
        function wt_preload_tooltip() {
	    $img = jQuery("<img/>");
	    $img.attr("src", '<?php echo WordTrailsGlobal::$urlpath; ?>imgs/tooltip/tooltip_fill.png');
	    $img.attr("src", '<?php echo WordTrailsGlobal::$urlpath; ?>imgs/tooltip/tooltip_top.png');
	    $img.attr("src", '<?php echo WordTrailsGlobal::$urlpath; ?>imgs/tooltip/tooltip_bottom.png');
	    $img.attr("src", '<?php echo WordTrailsGlobal::$urlpath; ?>imgs/tooltip/tooltip_vpb_top.png');
	    $img.attr("src", '<?php echo WordTrailsGlobal::$urlpath; ?>imgs/tooltip/tooltip_vpb_bottom.png');
	}
    //-->
    </script>
<?php
}
add_action("wp_head", "wt_tooltip_head");
wp_register_script("bgiframe", WordTrailsGlobal::$urlpath . "js/jquery.bgiframe.js", array("jquery"));
wp_register_script("dimensions", WordTrailsGlobal::$urlpath . "js/jquery.dimensions.js", array("jquery"));
wp_register_script("tooltip", WordTrailsGlobal::$urlpath . "js/jquery.tooltip.js", array("jquery", "bgiframe", "dimensions"));
wp_enqueue_script("WordTrailsToolTip", WordTrailsGlobal::$urlpath . "js/tooltip.js", array("tooltip"));
?>