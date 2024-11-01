<?php
require_once "WordTrailsGlobal.inc";

function wp_wt_add_weekly_schedule($schedules = array()) {
    $weekly = array('weekly' => array( 'interval' => 604800, 'display' => __('Once Weekly')));
    $every = array('everyload' => array( 'interval' => 0, 'display' => __('Every Load')));
    return array_merge($schedules, $weekly, $every);
}

function wp_wt_edit_form_advanced() {
    global $post;
    add_meta_box("trail_blazing", "WordTrails", array("WordTrailsGlobal", "editFormBox"), "post", "advanced");
}

function wp_wt_widget_init() {
    register_sidebar_widget('Trails Navigation', 'wp_wt_widget_worms_eye_view');
    register_widget_control('Trails Navigation', array("WordTrailsWidget", "widget_options"));
}
function wp_wt_widget_worms_eye_view($args) {
    global $wp_query;
    if ((is_single() || is_page()) && !is_page(WordTrailsGlobal::get_option(WordTrailsGlobal::TRAILS_PAGE_OPTION))) {
        WordTrailsUtilities::makeWidget($wp_query->post->ID,$args);
    }
}

function wp_wt_template_redirect($args) {
    if (is_null(WordTrailsGlobal::get_option(WordTrailsGlobal::TRAILS_PAGE_OPTION))) return;
    if (is_page(WordTrailsGlobal::get_option(WordTrailsGlobal::TRAILS_PAGE_OPTION))) {
	include "hijack.inc";
    }
}

function wp_wt_head() {
    WordTrailsUtilities::setStatics();
    wp_register_script("ltoh", WordTrailsGlobal::$urlpath . "js/ltoh.js", array("jquery"));
    $sidebars = get_option('sidebars_widgets');
    if (is_page(WordTrailsGlobal::get_option(WordTrailsGlobal::TRAILS_PAGE_OPTION))) {
	if (!empty($_GET["print"])) {
	    wp_enqueue_script("jquery-ui-draggable");
	    wp_enqueue_script("jquery-ui-sortable");
	}
	include_once "tooltip.inc";
	wp_enqueue_script("ltoh");
	//wp_enqueue_script("WordTrailsWidget", WordTrailsGlobal::$urlpath . "js/widget.js", array("jquery", "tooltip"));
	//if (function_exists("wp_enqueue_style")) {
	    //wp_enqueue_style("WordTrailsIndex", WordTrailsGlobal::$urlpath . "css/trailindex.css");
	//} else {
	    add_action("wp_head", "wp_wt_index_css");
	//}
    } elseif (is_array($sidebars)) {
	foreach ($sidebars as $bar) {
	    if (in_array('trails-navigation', (array)$bar)) {
		include_once "tooltip.inc";
		wp_enqueue_script("WordTrailsWidget", WordTrailsGlobal::$urlpath . "js/widget.js", array("ltoh"));
		if (function_exists("wp_enqueue_style")) {
		    wp_enqueue_style("WordTrailsWidget", WordTrailsGlobal::$urlpath . "css/widget.css");
		} else {
		    add_action("wp_head", "wp_wt_widget_css");
		}
		break;
	    }
	}
    }
}

function wp_wt_widget_css() {
    WordTrailsUtilities::setStatics();?>

<link rel="stylesheet" type="text/css" href="<?php echo WordTrailsGlobal::$urlpath; ?>css/widget.css" /><?php
}

function wp_wt_index_css() {
    WordTrailsUtilities::setStatics();?>

<link rel="stylesheet" type="text/css" href="<?php echo WordTrailsGlobal::$urlpath; ?>css/trailindex.css" /><?php
}

function wp_wt_track_hits() {
    global $wp_the_query;
    if (is_single()||is_page()) WordTrailsGlobal::trackHits($wp_the_query->post->ID);
}

function wp_wt_admin_init() {
    WordTrailsUtilities::setStatics();
}

function wp_wt_slug_tags() {
    add_rewrite_tag("%trail_slug%", ".*");
    add_rewrite_tag("%wt_action%", "wt_.*");
}

function wp_wt_slug_rules_filter($rules) {
    global $wp_rewrite;
    $newrules = array();
    
    $index_pid = WordTrailsGlobal::get_option(WordTrailsGlobal::TRAILS_PAGE_OPTION);
    $index_slug = get_post_field('post_name', $index_pid);
    $newrules[$index_slug . "/page/?([0-9]{1,})/?$"] = "index.php?page_id=" . $index_pid . "&paged=\$matches[1]";
    $newrules[$index_slug . "/(.+?)/?$"] = "index.php?page_id=" . $index_pid . "&trail_slug=\$matches[1]";
    
    $permastruct = get_option('permalink_structure');
    $structs = split("/", $permastruct);
    $pattern = "";
    $redirect = "index.php?";
    $i=1;
    foreach ($structs as $struct) {
	if (empty($struct)) continue;
	$structid = array_search($struct, $wp_rewrite->rewritecode);
	if (false === $structid) {
	    $pattern .= $struct . "/";
	    continue;
	}
	$pattern .= $wp_rewrite->rewritereplace[$structid] . "/";
	$redirect .= $wp_rewrite->queryreplace[$structid] . "\$matches[$i]&";
	$i++;
    }
    $newrules[$pattern . "on-trail/(.+?)/?$"] = $redirect . "trail_slug=\$matches[$i]";
    $newrules["(.+?)/on-trail/(.+?)/?$"] = "index.php?pagename=\$matches[1]&trail_slug=\$matches[2]";
    
    foreach (array("xml", "print") as $type) {
	$newrules[WordTrailsGlobal::$redirect_query_slugs[$type] . "/(.+?)/?$"] = "index.php?wt_action=" . WordTrailsGlobal::$redirect_query_strings[$type] . "&trail_slug=\$matches[1]";
    }
    
    return $newrules + $rules;
}

add_action('admin_menu', array("WordTrailsUtilities", "installMenus"));
add_action('shutdown', array("WordTrailsGlobal", "sessionStore"));
add_action('template_redirect', 'wp_wt_head');
add_action('loop_start', 'wp_wt_track_hits');
add_action('template_redirect', 'wp_wt_template_redirect');
add_action('parse_request', array("WordTrailsGlobal", "capture_redirect"));
add_action('edit_form_advanced', 'wp_wt_edit_form_advanced');
add_action('save_post', array("WordTrailsGlobal", "blazeTrail"));
add_action('delete_post', array("WordTrailsGlobal", "deletePID"));
add_action('trashed_post', array("WordTrailsGlobal", "trashedPID"));
add_action('untrashed_post', array("WordTrailsGlobal", "untrashedPID"));
add_action('widgets_init', 'wp_wt_widget_init');
add_filter('cron_schedules', 'wp_wt_add_weekly_schedule');
add_action(WordTrailsGlobal::ANALYTICS_TRANSMIT_OPTION, array("WordTrailsGlobal", "sendAnalyticsToTrailMeme"));
add_action(WordTrailsGlobal::ANALYTICS_CRON, array("WordTrailsAnalytics", "cronCrunch"));
add_action('set_current_user', array("WordTrailsUtilities", "setStatics"));
add_action("admin_init", "wp_wt_admin_init");
add_filter("init", "wp_wt_slug_tags");
add_filter("rewrite_rules_array", "wp_wt_slug_rules_filter");

if (defined("DOING_AJAX"))
    if (DOING_AJAX)
        require_once "ajax.inc";
?>