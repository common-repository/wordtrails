<?php
require_once "WordTrailsGlobal.inc";
WordTrailsUtilities::unScheduleAnalytics();
WordTrailsUtilities::unScheduleCronCrunch();
WordTrailsUtilities::delCapabilities();

global $wp_rewrite;
$wp_rewrite->flush_rules();
?>