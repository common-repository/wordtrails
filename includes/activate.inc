<?php
require_once "WordTrailsGlobal.inc";
global $wpdb, $wp_rewrite;
WordTrailsUtilities::updateNewOptions();
WordTrailsGlobal::destroyFirst();

require_once "dbstruct.inc";

$sidebars = get_option('sidebars_widgets');
if (is_array($sidebars) && !empty($sidebars)) {
    foreach ($sidebars as &$bar) {
        if (in_array('trailmeme-navigation', (array)$bar)) {
            array_splice($bar, array_search("trailmeme-navigation", $bar), 1, (in_array("trails-navigation", (array)$bar) ? null : "trails-navigation"));
            update_option('sidebars_widgets', $sidebars);
        }
    }
}

WordTrailsUtilities::scheduleAnalytics();
WordTrailsUtilities::scheduleCronCrunch();
WordTrailsUtilities::pushDefaultControls();
WordTrailsUtilities::addCapabilities();
$wp_rewrite->flush_rules();
?>