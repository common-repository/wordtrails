<?php
require_once "WordTrailsGlobal.inc";

class WordTrailsUtilities {
    public static $statics_set = false;
    public static $tables_set = false;
    
    private static $checking_title = false;

    public static function reqClasses() {
        foreach (WordTrailsGlobal::$node_classes as $where => $types) {
            foreach ($types as $type => $class) {
                require_once $class.".inc";
            }
        }
	require_once "WordTrailsWidget.inc";
        require_once "WordTrailsTagManager.inc";
        require_once "WordTrailsMenu.inc";
        require_once "WordTrailsAnalytics.inc";
	require_once "WordTrailsControlManager.inc";
    }

    //---------OPTIONS----------
    public static function option_exists( $setting ) {
        $alloptions = wp_load_alloptions();
	if ( isset( $alloptions[$setting] ) ) return true;
        if ( false !== wp_cache_get( $setting, 'options' ) ) return true;
        return false;
    }

    public static function initialize_option ( $setting, $default ) {
        if (self::option_exists($setting)) return get_option($setting);
        add_option($setting, $default);
        return $default;
    }
    
    public static function updateNewOptions() {
	if (!self::option_exists(WordTrailsGlobal::ALL_OPTIONS)) WordTrailsGlobal::get_all_options();
	else {
	    $options = get_option(WordTrailsGlobal::ALL_OPTIONS);
	    foreach($options as $key => $val) {
		WordTrailsGlobal::$options[$key] = $val;
	    }
	    update_option(WordTrailsGlobal::ALL_OPTIONS, WordTrailsGlobal::$options);
	}
	$site_hash = WordTrailsGlobal::get_option(WordTrailsGlobal::SITE_HASH_OPTION);
	if (is_null($site_hash) || empty($site_hash))
	    WordTrailsGlobal::update_option(WordTrailsGlobal::SITE_HASH_OPTION, self::newHash());
    }
    //-------ENDOPTIONS---------

    //-----------MISC-----------
    public static function newHash() {
        global $wpdb;
        return $wpdb->get_var("SELECT UUID()");
    }

    public static function isHash($hash) {
        $expl_hash = explode("-", $hash);
        if (count($expl_hash) == 5 && strlen(implode("",$expl_hash)) == 32)
            return true;
        return false;
    }

    public function stampToAbbr($mysqlTimestamp, $human_only = false) {
        $time = strtotime($mysqlTimestamp);
        $long = date(__('Y/m/d g:i:s A'), $time);
        $h_time = date(__('Y/m/d'), $time);
        if (abs(time() - $time) < 86400) {
            if (time() - $time < 0)
                $h_time = sprintf( __('%s from now'), human_time_diff( $time ) );
            else
		$h_time = sprintf( __('%s ago'), human_time_diff( $time ) );
        }
	if ($human_only) return $h_time;
        return "<abbr title=\"". $long ."\">" . $h_time . "</abbr>";
    }

    public static function wrap_with_quotes($a) {
	return "\"" . $a . "\"";
    }
    
    public static function plain_quotes($string) {
	$search = array(chr(145), chr(146), chr(147), chr(148), chr(151));
	$replace = array("'", "'", '"', '"', '-');
	$string = str_replace($search, $replace, $string);
	$search = array('&lsquo;', '&rsquo;', '&ldquo;', '&rdquo;', '&mdash;');
	$string = str_replace($search, $replace, $string);
	$search = array("â€" . chr(152), "â€" . chr(153), 'â€' . chr(156), 'â€' . chr(157),  'â€"');
	return str_replace($search, $replace, $string);
    } 

    public static function is_serialized_trail($data) {
	if (!is_serialized($data)) return false;
	foreach (WordTrailsGlobal::$node_classes as $where => $types) {
	    if (strstr($data, $types["trail"]))
		return true;
	}
	return false;
    }

    public static function filter_unsaved_only($n) {
	if (is_serialized($n)) return true;
	if (is_object($n)) {
	    if ($n->hasUnsaved()) return true;
	    return false;
	}
	$node = WordTrailsGlobal::getNode($n, false);
	if (!is_object($node)) return false;
	if ($node->hasUnsaved()) return true;
	return false;
    }


    public static function sort_roles($a, $b) {
	if ($a["numcaps"] == $b["numcaps"]) {
	    return 0;
	}
	return ($a["numcaps"] > $b["numcaps"]) ? -1 : 1;
    }


    public static function is_newer_version($override = true) {
        $installed = split(".", WordTrailsGlobal::get_option(WordTrailsGlobal::VERSION_OPTION));
        $mine = split(".", WordTrailsGlobal::VERSION);
        for ($i = 0; $i < 3; $i++) {
            if ($installed[$i] == $mine[$i]) continue;
            if ($installed[$i] < $mine[$i]) {
                if ($override) WordTrailsGlobal::update_option(WordTrailsGlobal::VERSION_OPTION, WordTrailsGlobal::VERSION);
                return true;
            }
            if ($installed[$i] > $mine[$i]) return false;
        }
        return false;
    }


    public static function setTables() {
	if (self::$tables_set) return false;
        global $wpdb;
	WordTrailsGlobal::$tables = (object)array(
            "node" => $wpdb->prefix . "wt_node",
            "node_rel" => $wpdb->prefix . "wt_node_relationship",
            "node_tag" => $wpdb->prefix . "wt_node_tag",
            "tag" => $wpdb->prefix . "wt_tag",
            "external" => $wpdb->prefix . "wt_external",
            "push" => $wpdb->prefix . "wt_trail_push",
            "usage" => $wpdb->prefix . "wt_usage_analytics",
            "usage_push" => $wpdb->prefix . "wt_usage_push",
            "print" => $wpdb->prefix . "wt_print_analytics",
            "print_push" => $wpdb->prefix . "wt_print_push",
	    "control" => $wpdb->prefix . "wt_control",
	    "rel_control" => $wpdb->prefix . "wt_relationship_control",
	    "old_slug" => $wpdb->prefix . "wt_old_slug",
	    "mid_crunch" => $wpdb->prefix . "wt_mid_crunch",
	    "high_crunch" => $wpdb->prefix . "wt_high_crunch"
        );
	self::$tables_set = true;
	return true;
    }
    
    public static function pushDefaultControls() {
	global $wpdb;
	$sql = "REPLACE INTO " . WordTrailsGlobal::$tables->control . " (`ControlID`, `Term`, `Name`, `Image`, `isForward`, `Implies`) VALUES
	(1, 'auto', '(Automatic)', 'straight.png', 1, 2),
	(2, 'auto', '(Automatic)', 'straight_back.png', 0, 1),
	(3, 'straight', 'Straight', 'straight.png', 1, 4),
	(4, 'straight_back', 'Straight', 'straight_back.png', 0, 3),
	(5, 'curve_up', 'Curve Up', 'curve_up.png', 1, 6),
	(6, 'curve_down_back', 'Curve Down', 'curve_down_back.png', 0, 5),
	(7, 'curve_down', 'Curve Down', 'curve_down.png', 1, 8),
	(8, 'curve_up_back', 'Curve Up', 'curve_up_back.png', 0, 7);";
	$wpdb->query($sql);
    }
    public static function addCapabilities() {
	require_once trailingslashit(ABSPATH . WPINC) ."capabilities.php";
	global $wp_roles;
	$role_caps = array();
	foreach ($wp_roles->roles as $role_name => $role_array) {
	    $role_caps[] = array("key" => $role_name, "name" => $role_array["name"], "numcaps" => count($role_array["capabilities"]));
	}
	usort($role_caps, array("WordTrailsUtilities", "sort_roles"));
	$min_role = strtolower(WordTrailsGlobal::get_option(WordTrailsGlobal::MIN_ROLE_SAVE));
	$add = true;
	for ($i=0; $i<count($role_caps); $i++) {
	    if ($add) $wp_roles->add_cap($role_caps[$i]["key"], WordTrailsGlobal::CAP_SAVE);
	    else $wp_roles->remove_cap($role_caps[$i]["key"], WordTrailsGlobal::CAP_SAVE);
	    if (strtolower($role_caps[$i]["key"]) == $min_role || strtolower($role_caps[$i]["name"]) == $min_role) $add = false;
	}
    }
    
    public static function delCapabilities() {
	require_once trailingslashit(ABSPATH . WPINC) ."capabilities.php";
	global $wp_roles;
	foreach ($wp_roles->roles as $role_name => $role_array) {
	    $wp_roles->remove_cap($role_name, WordTrailsGlobal::CAP_NEW);
	    $wp_roles->remove_cap($role_name, WordTrailsGlobal::CAP_MOD);
	    $wp_roles->remove_cap($role_name, WordTrailsGlobal::CAP_SAVE);
	}
    }
    
    public static function getHijackTitle($override = false) {
	if (self::$checking_title) return null;
	if ($override || is_null(WordTrailsGlobal::$hijack_title)) {
	    self::$checking_title = true;
	    $ID = WordTrailsGlobal::get_option(WordTrailsGlobal::TRAILS_PAGE_OPTION);
	    if (is_numeric($ID)) {
		WordTrailsGlobal::$hijack_title = get_the_title((int)$ID);
		self::$checking_title = false;
	    }
	    else
		return null;
	}
	return WordTrailsGlobal::$hijack_title;
    }
    

    public static function setStatics() {
        self::setTables();
	if (self::$statics_set) return false;
	load_plugin_textdomain(WordTrailsGlobal::DOMAIN, trailingslashit(WP_PLUGIN_DIR).dirname(dirname(plugin_basename(__FILE__))));
        if (session_id() == "") session_start();

	WordTrailsGlobal::$abspath = trailingslashit(WP_PLUGIN_DIR) . dirname(dirname(plugin_basename(__FILE__))) . "/";
	WordTrailsGlobal::$urlpath = trailingslashit(WP_PLUGIN_URL) . dirname(dirname(plugin_basename(__FILE__))) . "/";

        self::reqClasses();

	self::$statics_set = true;
	return true;
    }

    public static function makeWidget($pid, $args) {
        $widg = new WordTrailsWidget($pid, $args);
        $widg->displayWidget();
    }
    
    /* for future version
    public static function &makePrintNode($trailHash) {
	$trail = WordTrailsGlobal::getNode($trailHash);
	if (is_object($trail)) {
	    $print = $trail->getPrintNode();
	    if (is_object($print)) return $print;
	}
	$print = WordTrailsGlobal::createNewNode(true, "print");
	$print->fromTrail($trailHash);
	return $print;
    } */

    public static function installMenus() {
	WordTrailsGlobal::makeInstance();
	WordTrailsGlobal::$instance->menu->installMenus();
	global $wp_roles;
    	//superprint($wp_roles->roles, true);
    }
    public static function trailsMenu() {
	WordTrailsGlobal::$instance->menu->trailsMenu();
    }
    public static function add_new_trail() {
	WordTrailsGlobal::$instance->menu->add_new_trail();
    }
    public static function settingsMenu() {
	WordTrailsGlobal::$instance->menu->settingsMenu();
    }
    public static function trailMemeMenu() {
	WordTrailsGlobal::$instance->menu->trailMemeMenu();
    }
    public static function analyticsMenu() {
	WordTrailsGlobal::$instance->menu->analyticsMenu();
    }
    public static function helpMenu(){
	WordTrailsGlobal::$instance->menu->helpMenu();
    }
    public static function menu_head() {
	wp_enqueue_script("wt_menu", WordTrailsGlobal::$urlpath . "js/menu.js", wt_menu_prereqs());
	if (WordTrailsGlobal::$instance->menu->isMenuPage("Analytics")) {
	    include_once "tooltip.inc";
	    wp_enqueue_script("wt_analytics", WordTrailsGlobal::$urlpath . "js/analytics.js", "wt_menu");
	    wt_tooltip_head(); ?>
    <style type="text/css">
	<!--
	.waiting {
	    background-image: url('<?php echo WordTrailsGlobal::$urlpath; ?>imgs/waiting.png');
	    background-repeat: repeat-x;
	}
	.loading {
	    background-image: url('<?php echo WordTrailsGlobal::$urlpath; ?>imgs/loading.gif');
	    background-repeat: repeat-x;
	}
	-->
    </style><?php
	}
	if (WordTrailsGlobal::$instance->menu->isMenuPage("Help")) {
	    if (function_exists("wp_print_styles")) {
		wp_print_styles("dashboard");
	    } else { ?>

<link rel="stylesheet" type="text/css" href="<?php echo get_option("site_url"); ?>wp-admin/css/dashboard.css" /><?php
	    }
	}
	WordTrailsGlobal::$instance->menu->insertCSS();
    }
    public static function requireAjaxURL() {
	add_action('admin_footer', array(__CLASS__,'printAjaxURL'));
	add_action('get_footer', array(__CLASS__, 'printAjaxURL'));
    }
    public static function printAjaxURL() { ?>

<script type='text/javascript'>
/* <![CDATA[ */
	var ajax_url = "<?php echo admin_url("admin-ajax.php"); ?>";
/* ]]> */
</script>
<?php
    }
    public static function warnTrailPage() {
	return (is_null(WordTrailsGlobal::get_option(WordTrailsGlobal::TRAILS_PAGE_OPTION)) && WordTrailsGlobal::get_option(WordTrailsGlobal::WARN_TRAIL_PAGE));
    }
    public static function warnWidget() {
	$widget = false;
	//double-check sidebars (should already be checked)
	$sidebars = get_option('sidebars_widgets');
	if (!is_array($sidebars) || empty($sidebars)) return false;
	foreach ($sidebars as $bar) {
	    if (in_array('trails-navigation', (array)$bar)) {
		$widget = true;
		break;
	    }
	}
	return ((!$widget) && WordTrailsGlobal::get_option(WordTrailsGlobal::WARN_WIDGET));
    }
    public static function warnNoSidebar() {
	$sidebars = get_option('sidebars_widgets');
	if ((!is_array($sidebars) || empty($sidebars)) && WordTrailsGlobal::get_option(WordTrailsGlobal::WARN_SIDEBAR)) return true;
	return false;
    }
    
    public static function warnTrailMeme() {
	return false;//(is_null(WordTrailsGlobal::get_option(WordTrailsGlobal::TRAILMEME_KEY_OPTION)) && WordTrailsGlobal::get_option(WordTrailsGlobal::WARN_TRAILMEME));
    }

    public static function scheduleAnalytics() {
	wp_schedule_event(time()+5, 'weekly', WordTrailsGlobal::ANALYTICS_TRANSMIT_OPTION);
    }

    public static function unScheduleAnalytics() {
	wp_clear_scheduled_hook(WordTrailsGlobal::ANALYTICS_TRANSMIT_OPTION);
    }
    
    public static function scheduleCronCrunch() {
	self::unScheduleCronCrunch();
	wp_schedule_event(time()+5,"hourly", WordTrailsGlobal::ANALYTICS_CRON);
    }
    public static function unScheduleCronCrunch() {
	wp_clear_scheduled_hook(WordTrailsGlobal::ANALYTICS_CRON);
    }
    public static function reScheduleCronCrunch() {
	self::unScheduleCronCrunch();
	self::scheduleCronCrunch();
    }
    
    public static function browseTime($depth) {
	$depth = (int)$depth;
	$mins = $depth*5;
	if ($mins < 60) return "$mins mins";
	$hrs = round($mins/60);
	if ($hrs < 2) return "About 1 hour";
	return "About $hrs hours";
    }
    
    public static function truncateLongName($name, $length = 20, $append = "...") {
	if (strlen($name) > $length + $append) {
	    $name = substr($name, 0, $length) . $append;
	}
	return $name;
    }
    
    public static function isUniqueSlug($newslug) {
	global $wpdb;
	$sql = "SELECT Slug FROM " . WordTrailsGlobal::$tables->node . " WHERE Slug LIKE '" . substr($newslug, 0, -3) . "%';";
	$slugs = $wpdb->get_col($sql);
	foreach ($slugs as $slug) {
	    if ($slug == $newslug) return false;
	}
	foreach(WordTrailsGlobal::$instance->nodes as $hash => &$node) {
            if (self::is_serialized_trail($node)) $node = unserialize($node);
            if (is_object($node)) {
                if ($newslug == $node->getSlug()) return false;
            }
        }
	return true;
    }
    
    public static function recordOldSlug($slug, $hash) {
	$sql = "INSERT INTO " . WordTrailsGlobal::$tables->old_slug . " (OldSlugID,Slug,Hash) VALUES (NULL, %s, %s) ON DUPLICATE KEY UPDATE OldSlugID = LAST_INSERT_ID(OldSlugID), Hash = VALUES(Hash);";
	global $wpdb;
	return $wpdb->query($wpdb->prepare($sql, $slug, $hash));
    }
    
    public static function deleteSlugs($hash) {
	$sql = "DELETE FROM " . WordTrailsGlobal::$tables->old_slug . " WHERE Hash = %s";
	global $wpdb;
	return $wpdb->query($wpdb->prepare($sql, $hash));
    }    
    //---------ENDMISC----------

    //----------URLS-----------
    public static function permalink($pid = null, $tid = null) {
	$site_pid = (int)WordTrailsGlobal::get_option(WordTrailsGlobal::TRAILS_PAGE_OPTION);
        if (is_null($pid)) {
            $pid = $site_pid;
        } else {
            $pid = (int)$pid;
        }
        $url = get_permalink($pid);
        if ($url === false) {
            $url = get_option('home');
            return $url;
        }
	$permalink = get_option('permalink_structure');
        if (!is_null($tid)) {
	    if ( '' != $permalink ) {
		$trail = WordTrailsGlobal::getNode(WordTrailsGlobal::getNodeHashFromID($tid));
		if (!is_object($trail)) return self::ugly_permalink($pid, $tid);
		$slug = $trail->getSlug();
		if (is_null($slug) || empty($slug)) return self::ugly_permalink($pid, $tid);
		if ($pid == $site_pid) {
		    $url = user_trailingslashit(trailingslashit($url) . $slug);
		} else {
		    $url = user_trailingslashit(trailingslashit($url) . "on-trail" . "/" . $slug);
		}
		return $url;
	    } else return self::ugly_permalink($pid, $tid);
        }
        return $url;
    }
    
    public static function ugly_permalink($pid = null, $tid = null) {
	$site_pid = (int)WordTrailsGlobal::get_option(WordTrailsGlobal::TRAILS_PAGE_OPTION);
        if (is_null($pid)) {
            $pid = $site_pid;
        } else {
            $pid = (int)$pid;
        }
	$url = get_permalink($pid);
        if ($url === false) {
            $url = get_option('home');
            return $url;
        }
	if (!is_null($tid)) {
	    if (false === strpos($url, "?")) {
		$url .= "?";
	    } else {
		$url .= "&";
	    }
	    $url .= 't=' . $tid;
        }
        return $url;
    }

    public static function redirect_link_to($qstr, $val = null) {
        $url = get_option('home') . "/";
	$orig_val = $val;
	$permalink = get_option('permalink_structure');
	if ( '' != $permalink  && in_array($qstr, array_keys(WordTrailsGlobal::$redirect_query_slugs))) {
	    $url .= WordTrailsGlobal::$redirect_query_slugs[$qstr];
	    if (is_numeric($val)) {
		$val = WordTrailsGlobal::getNodeHashFromID($val);
	    }
	    if (self::isHash($val)) {
		$node = WordTrailsGlobal::getNode($val);
		$val = $node->getSlug();
		if (is_null($val) || empty($val)) return self::ugly_redirect_link_to($qstr, $orig_val);
	    }
	    if (!is_null($val) && !empty($val)) {
		$url .= "/" . $val;
	    }
	    return user_trailingslashit($url);
	} else return self::ugly_redirect_link_to($qstr, $orig_val);
    }
    public static function ugly_redirect_link_to($qstr, $val = null) {
        $url = get_option('home') . "/";
	if (in_array($qstr, array_keys(WordTrailsGlobal::$redirect_query_strings))) {
            $url .= "?" . WordTrailsGlobal::$redirect_query_strings[$qstr] . "=";
            if (!is_null($val))
                $url .= $val;
        }
        return $url;
    }
    
    public static function appendVar($url, $varname, $var, $direct_to_xhtml = true) {
	if (false === strpos($url, "?")) {
	    $url .= "?";
	} else {
	    $url .= "&";
	    if ($direct_to_xhtml) $url .= "amp;";
	}
	return $url . $varname . "=" . $var;
    }
    //--------ENDURLS----------

    //-----------XML-----------
    public static function createBasicXMLNode($name, $value = null, &$dom = null) {
        if (is_null($dom))
            $dom = new DOMDocument("1.0");
        $node = $dom->createElement($name);
        if (!is_null($value) && (!empty($value) || $value === 0)) {
            $node->appendChild($dom->createTextNode($value));
        }
        return $node;
    }

    public static function explodeToXMLNodes($bigname, $littlename = "tag", $haystack, $split = null, &$dom = null) {
        if (is_null($dom))
            $dom = new DOMDocument("1.0");
        $node = $dom->createElement($bigname);
        if (is_string($haystack)) {
            if (!is_null($split)) {
                $haystack = explode($split, $haystack);
                $haystack = array_map(trim, $haystack);
            } else {
                $haystack = array();
            }
        }
        if (empty($littlename)) $littlename = "tag";
        if (!is_array($haystack)) echo "not";
        foreach ($haystack as $needle) {
            if (!empty($needle)) {
                $temp = $dom->createElement($littlename);
                $temp->appendChild($dom->createTextNode($needle));
                $node->appendChild($temp);
            }
        }
        return $node;
    }

    public static function extractXMLNode($node, &$store = null) {
        if ($node->hasChildNodes()) {
            $val = null;
            if (count($node->childNodes)== 1 && $node->firstChild->nodeType == XML_TEXT_NODE) {
                $val = $node->firstChild->nodeValue;
                if (is_numeric($val)) {
                    $val = round($val, 2);
                }
            } else if (count($node->childNodes) > 0) {
                $doc = new DOMDocument("1.0");
                $doc->formatOutput = false;
                $doc->appendChild($doc->importNode($node,true));
                foreach ($node->childNodes as $child) {
                    $val[] = self::extractXMLNode($child);
                }
            }
            if (!is_null($val)) {
                if (!is_null($store))
                    $store = $val;
                return $val;
            }
        }
        return null;
    }

    public static function prepareForXML(&$dom = null) {
        if (is_null($dom)) $dom = new DOMDocument("1.0");
        $root = $dom->createElement("traildoc");
        $root->setAttribute("application", "WordPress");
        $root->setAttribute("version", WordTrailsGlobal::XMLVERSION);
        $dom->appendChild($root);
        return array($dom, $root);
    }
    //---------ENDXML----------
    
    /**
    * sum values in array
    *
    * @param array $arr
    * @param string [optional]$index
    * @return int result
    */
    public static function array_sum_key( $arr, $index = null ){
	if(!is_array( $arr ) || sizeof( $arr ) < 1){
	    return 0;
	}
	$ret = 0;
	foreach( $arr as $id => $data ){
	    if( isset( $index )  ){
		$ret += (isset( $data[$index] )) ? $data[$index] : 0;
	    }else{
		$ret += $data;
	    }
	}
	return $ret;
    }
    /**
    * count values in array
    *
    * @param array $arr
    * @param string [optional]$index
    * @return int result
    */
    public static function array_count_key( $arr, $index ){
	if(!is_array( $arr ) || sizeof( $arr ) < 1){
	    return 0;
	}
	$ret = 0;
	foreach( $arr as $id => $data ){
	    $ret += (isset( $data[$index] ) && $data[$index] > 0) ? 1 : 0;
	}
	return $ret;
    }

    //--------SORTING----------
    public static function sort_hash($a, $b, $first = null) {
        if (!is_null($first) && $a == $first) return -1;
        if (!is_null($first) && $b == $first) return 1;
        if ($a == $b) return 0;
        return strcasecmp($a, $b);
    }

    public static function sort_trails($a, $b) {
        $na = WordTrailsGlobal::getNode($a, false);
        $nb = WordTrailsGlobal::getNode($b, false);
        if (is_object($na) && !is_object($nb)) return -1;
        if (is_object($nb) && !is_object($na)) return 1;
        $first = null;
        WordTrailsGlobal::makeInstance();
        if (!is_null(WordTrailsGlobal::$instance->current_trail_id))
            $first = WordTrailsGlobal::getNodeHashFromID(WordTrailsGlobal::$instance->current_trail_id);
        return self::sort_hash($a, $b, $first);
    }

    public static function sort_default_children($a, $b) {
        if ($a["default"]) return -1;
        if ($b["default"]) return 1;
        return 0;
    }

    public static function array_multi_sort($a, $b) {
	global $wp_wt_sort;
	if (!isset($a[$wp_wt_sort]) && !isset($b[$wp_wt_sort])) return 0;
	if (!isset($a[$wp_wt_sort])) return -1;
	if (!isset($b[$wp_wt_sort])) return 1;
	if (is_numeric($a[$wp_wt_sort])) {
	    if ($a[$wp_wt_sort] < $b[$wp_wt_sort]) return -1;
	    if ($a[$wp_wt_sort] > $b[$wp_wt_sort]) return 1;
	    return 0;
	} else return strcasecmp($a[$wp_wt_sort], $b[$wp_wt_sort]);
    }

    public static function array_object_sort($a, $b) {
	global $wp_wt_sort;
	if (!isset($a->{$wp_wt_sort}) && !isset($b->{$wp_wt_sort})) return 0;
	if (!isset($a->{$wp_wt_sort})) return -1;
	if (!isset($b->{$wp_wt_sort})) return 1;
	if (is_numeric($a->{$wp_wt_sort})) {
	    if ($a->{$wp_wt_sort} < $b->{$wp_wt_sort}) return -1;
	    if ($a->{$wp_wt_sort} > $b->{$wp_wt_sort}) return 1;
	    return 0;
	} else return strcasecmp($a->{$wp_wt_sort}, $b->{$wp_wt_sort});
    }
    
    public static function onlyWEV($a) {
	if ($a->type == "BEV" || empty($a->NodeHash)) return false;
	return true;
    }
    
    public static function extractNodeHash($a) {
	return $a->NodeHash;
    }
    //------ENDSORTING---------
}

function superprint($mixed, $pre = false) {
    if ($pre) echo "<pre>";
    print_r($mixed);
    if ($pre) echo "</pre>";
}
function superdump($mixed, $pre = false) {
    if ($pre) echo "<pre>";
    var_dump($mixed);
    if ($pre) echo "</pre>";
}
?>