<?php
function wp_wt_check_new_trails_page(){
    $val = $_REQUEST["val"];
    if (is_numeric($val))
	$query = query_posts("page_id=" . $val);
    else
        $query = query_posts("pagename=" . $val);
    die('checkNewTrailsPageResults(' . ((bool)count($query) ? "true" : "false") . ', ' . $_REQUEST['check_count'] . ', "' . $val . '");');
}
add_action("wp_ajax_wt_check_new_trails_page", "wp_wt_check_new_trails_page");

function wp_wt_set_default_child() {
    $parent = $_REQUEST["parent"];
    $child = $_REQUEST["child"];
    $parent = WordTrailsGlobal::getNode($parent);
    if (!is_object($parent)) die('alert("Fatal Error: Parent node not found");');
    $parent->setOnlyDefault($child);
    die('confirmDefaultChild("' . $child . '");');
}
add_action("wp_ajax_wt_set_default_child", "wp_wt_set_default_child");

function wp_wt_trailmeme_push() {
    WordTrailsGlobal::trailMemePush();
}
add_action("wp_ajax_wt_trailmeme_push", "wp_wt_trailmeme_push");

function wp_wt_store_expanded_analytics() {
    $expanded = $_POST["expanded"];
    $collapsed = $_POST["collapsed"];
    if (is_string($expanded)) $expanded = array_map(trim, explode(",",$expanded));
    if (is_string($collapsed)) $collapsed = array_map(trim, explode(",",$collapsed));
    if (!is_array($expanded)) $expanded = array();
    if (!is_array($collapsed)) $collapsed = array();
    $old_expanded = (array)WordTrailsGlobal::get_option(WordTrailsGlobal::ANALYTICS_EXPANDED_OPTION);
    $new_expanded = array_diff(array_unique(array_merge($old_expanded, $expanded)), $collapsed);
    WordTrailsGlobal::update_option(WordTrailsGlobal::ANALYTICS_EXPANDED_OPTION, $new_expanded);
}
add_action("wp_ajax_wt_store_expanded_analytics", "wp_wt_store_expanded_analytics");

function wp_wt_helpRSSDisplay() {
    $options = array(
	"title" => "TrailMeme News"
	,"url" => "http://feeds.feedburner.com/Blogtrailmemecom"
	,"link" => "http://blog.trailmeme.com/"
	,"items" => 5
	,"show_summary" => 1
	,"show_author" => 1
	,"show_date" => 1
    );
    echo "<div class='rss-widget'>";
    wp_widget_rss_output($options);
    echo "</div>";
    exit;
}
add_action("wp_ajax_wt_help_rss_display", "wp_wt_helpRSSDisplay");

function wp_wt_remove_admin_warning() {
    $option = $_POST["option"];
    $option_keys = array_keys(WordTrailsGlobal::$options);
    $option_key_id = array_search($option, $option_keys);
    if ( false === $option_key_id ) exit;
    $option = $option_keys[$option_key_id];
    WordTrailsGlobal::update_option($option, 0);
    exit;
}
add_action("wp_ajax_wt_remove_warning", "wp_wt_remove_admin_warning");

function wp_wt_check_warning() {
    WordTrailsGlobal::makeInstance();
    die(WordTrailsGlobal::$instance->menu->nextMessage());
}
add_action("wp_ajax_wt_check_warning", "wp_wt_check_warning");

function wp_wt_pre_load_high_level() {
    if (!isset($_POST["hash"]) || !WordTrailsUtilities::isHash($_POST["hash"])) die("There was an error trying to load from [{$_POST["hash"]}]");
    WordTrailsGlobal::makeInstance();
    $total = (int)$_POST["total"];
    $crunched = WordTrailsAnalytics::incrementalProgress($_POST["hash"], $total);
    $ret = array("crunched"=>$crunched, "percent"=>0, "total"=>$total);
    if ($total > 0) $ret["percent"] = round($crunched/$total,4)*100;
    $ret = (object)$ret;
    //trigger_error("returning:".var_export($ret, true) . " as " . json_encode($ret));
    die(json_encode($ret));
}
add_action("wp_ajax_wt_pre_load_high_level", "wp_wt_pre_load_high_level");

function wp_wt_get_trail_session_counts() {
    WordTrailsGlobal::makeInstance();
    die(json_encode(WordTrailsAnalytics::getSessionCountForTrails()));
}
add_action("wp_ajax_wt_get_trail_session_counts", "wp_wt_get_trail_session_counts");

function wp_wt_load_high_level() {
    if (!isset($_POST["hash"]) || !WordTrailsUtilities::isHash($_POST["hash"])) die("There was an error trying to load from [{$_POST["hash"]}]");
    WordTrailsGlobal::makeInstance();
    $menu = &WordTrailsGlobal::$instance->menu;
    die($menu->high_level_row($_POST["hash"]));
}
add_action("wp_ajax_wt_load_high_level", "wp_wt_load_high_level");

function wp_wt_load_mid_level() {
    if (!isset($_POST["hash"]) || !WordTrailsUtilities::isHash($_POST["hash"])) die("There was an error trying to load from [{$_POST["hash"]}]");
    trigger_error(print_r($_POST, true));
    WordTrailsGlobal::makeInstance();
    $menu = &WordTrailsGlobal::$instance->menu;
    die($menu->listSIDsForTrail($_POST["hash"]));
}
add_action("wp_ajax_wt_load_mid_level", "wp_wt_load_mid_level");

function wp_wt_load_details() {
    if (!isset($_POST["hash"]) || !WordTrailsUtilities::isHash($_POST["hash"])) die("There was an error trying to load from [{$_POST["hash"]}]");
    if (!isset($_POST["sid"])) die("There was an error trying to load from [" . $_POST["sid"] . "]");
    WordTrailsGlobal::makeInstance();
    $menu = &WordTrailsGlobal::$instance->menu;
    die($menu->detailedHitAndPrint($_POST["hash"],$_POST["sid"]));
}
add_action ("wp_ajax_wt_load_details", "wp_wt_load_details");
?>