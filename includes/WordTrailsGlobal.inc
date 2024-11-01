<?php
require_once ABSPATH . WPINC . '/functions.php';
require_once "WordTrailsUtilities.inc";
require_once "compatibility.inc";
/**
 * This is the centralization file for WordTrails
 *
 * This file contains the WordTrailsGlobal class, and creates a global instance.
 *
 * @package WordTrails
 * @since 0.0.2
 * @author Jesse Silverstein <jesse.silverstein@xerox.com>
 * @copyright 2008 XIG: SemPrint
 */

// {{{ WordTrailsGlobal
/**
 * Global class for all things WordTrails related
 *
 * This class will be the centralized location for all of the WordTrails
 * globally used variables and methods. Anything that WordTrails needs to do
 * more than once will be done from this class.
 *
 * @package WordTrails
 * @since 0.0.2
 * @author Jesse Silverstein <jesse.silverstein@xerox.com>
 * @copyright 2008 XIG: SemPrint
 * @global WordPress $wpdb WordPress database management class
 */
class WordTrailsGlobal {

    const VERSION = "0.9.8.1";
    const XMLVERSION = "2.2";
    const DOMAIN = "WordTrails";
    //public $is_setup;
    //public $is_shutdown = false;

    const CAP_NEW = "wp_wt_new_trails"; //deprecated
    const CAP_MOD = "wp_wt_mod_trails"; //deprecated
    const CAP_SAVE = "wp_wt_save_trails";
    const CHECK_ADMIN_CAP = "edit_plugins";

    const ALL_OPTIONS = "wordtrails_all_options";

    const VERSION_OPTION = "wt_version";
    const TRAILS_PAGE_OPTION = "wt_trails_page";
    const PREV_TRAILS_PAGE_OPTION = "wt_prev_trails_page";
    const TRAILMEME_KEY_OPTION = "wt_trailMeme_key";
    const SITE_HASH_OPTION = "wt_site_hash";
    const TAG_CLEANUP_OPTION = "wt_tag_cleanup_last";
    const ANALYTICS_COLLECT_OPTION = "wt_analytics_collect";
    const ANALYTICS_EXPANDED_OPTION = "wt_analytics_expanded";
    const ANALYTICS_TRANSMIT_OPTION = "wt_transmit_analytics";
    const MIN_ROLE_SAVE = "wt_min_role_save";
    const WARN_TRAIL_PAGE = "wt_warn_trail_page";
    const WARN_WIDGET = "wt_warn_widget";
    const WARN_SIDEBAR = "wt_warn_no_sidebar";
    const WARN_TRAILMEME = "wt_warn_trailmeme";
    const WIDGET_TITLE_OPTION = "wt_widget_title";
    
    const ANALYTICS_CRON = "wt_cron_crunch";

    public static $options = array(
        self::VERSION_OPTION => self::VERSION,
        self::TRAILS_PAGE_OPTION => null,
        self::PREV_TRAILS_PAGE_OPTION => null,
        self::TRAILMEME_KEY_OPTION => null,
        self::SITE_HASH_OPTION => null,
        self::TAG_CLEANUP_OPTION => null,
        self::ANALYTICS_EXPANDED_OPTION => array(),
        self::ANALYTICS_COLLECT_OPTION => 1,
        self::ANALYTICS_TRANSMIT_OPTION => 1,
        self::MIN_ROLE_SAVE => "author",
        self::WARN_TRAIL_PAGE => 1,
        self::WARN_WIDGET => 1,
        self::WARN_SIDEBAR => 1,
        self::WARN_TRAILMEME => 1,
        self::WIDGET_TITLE_OPTION => "Trails"
    );
    private static $option_defaults = true;
    
    public static $hijack_title = null;

    public static $redirect_query_strings = array("xml" => "wt_xml", "print" => "wt_print_trail", "ext" => "wt_ext_node", "save" => "wt_save", "tv_save" => "wt_tv_save", "revert" => "wt_revert");
    public static $redirect_query_slugs = array("xml" => "trail-xml", "print" => "print-trail");

    public static $instance = null;
    public static $controls = null;
    public $rel_controls = null;

    public $current_trail_id;
    public $blazing_trail_id = null;
    public $blazing_default_norel = false;

    public static $site_hash = null;

    public static $abspath;
    public static $urlpath;

    public static $node_classes = array("internal" => array(
        "node" => "WordTrailsPostNode"
        ,"trail" => "WordTrailsTrailNode"
        //,"section" => "WordTrailsSectionNode"
        //,"print" => "WordTrailsPrintNode"
    ), "external" => array(
        "node" => "WordTrailsExternalNode"
        ,"trail" => "WordTrailsExternalNode"
    ));

    public static $tables = array();
    public $tag_manager;
    public $menu;

    public $nodes = array();
    public $unsaved_parents = array(); // [$child_hash] = array($parent_hash1, $parent_hash2...)
    public $tempNodeCount = 0;
    public $nodeIDToHash = array();
    public $nodeHashToID = array();
    public $nodeSlugToHash = array();

    private $analyzer;

    public function __construct() {
        $this->__wakeup();
        $this->analyzer = new WordTrailsAnalytics();
        $this->menu = new WordTrailsMenu();
        $this->tag_manager = new WordTrailsTagManager();
        $this->rel_controls = new WordTrailsControlManager();
    }

    public function __sleep() {
        //echo "sleeping<br/>";
        //superdump($this, true);
        $this->nodes = array_filter($this->nodes, array("WordTrailsUtilities", "filter_unsaved_only"));
        foreach ($this->nodes as $hash => &$node) {
            if (!is_serialized($node)) {
                if (!is_object($node)) {
                    $this->nodes[$hash] = null;
                    continue;
                }
                $nodestr = serialize($node);
                if ($nodestr == "N;") $nodestr = null;
                $this->nodes[$hash] = $nodestr;
            }
        }
        return array_keys(get_object_vars($this));
    }
    public function __wakeup() {
	WordTrailsUtilities::setStatics();
    }
    public function __destruct() {
        self::$instance = null;
    }

    public static function makeInstance() {
        if (self::$instance != null) return false;
        if (session_id() == "") session_start();
        WordTrailsUtilities::reqClasses();
        if (!empty($_SESSION[get_option("home").'-wp_wt_global'])) {
            self::$instance = $_SESSION[get_option("home").'-wp_wt_global'];
            self::$instance = unserialize($_SESSION[get_option("home").'-wp_wt_global']);
            self::$controls = self::$instance->rel_controls;
            return 0;
        }
        self::$instance = new self;
        self::$controls = self::$instance->rel_controls;
        return true;
    }

    public static function &getInstance() {
        self::makeInstance();
        return self::$instance;
    }

    public static function &destroyFirst() {
        if (session_id() == "") session_start();
        if (is_object(self::$instance)) self::$instance->__destruct();
        self::$instance = null;
        unset($_SESSION[get_option("home").'-wp_wt_global']);
        $_SESSION[get_option("home").'-wp_wt_global'] = "";
        return self::getInstance();
    }

    public static function sessionStore() {
        if (self::$instance == null) return false;
        $_SESSION[get_option("home").'-wp_wt_global'] = serialize(self::$instance);
    }

    public static function get_all_options() {
        WordTrailsUtilities::setStatics();
        self::$options = WordTrailsUtilities::initialize_option(self::ALL_OPTIONS, self::$options);
        self::$option_defaults = false;
    }

    public static function update_option($name, $val) {
        if (self::$option_defaults) self::get_all_options();
        self::$options[$name] = $val;
        update_option(self::ALL_OPTIONS, self::$options);
    }

    public static function get_option($name) {
        if (self::$option_defaults) self::get_all_options();
        return self::$options[$name];
    }

    public static function hashExists($hash) {
        if (!WordTrailsUtilities::isHash($hash)) return false;
        self::makeInstance();
        if (isset(self::$instance->nodes[$hash])) return true;
        global $wpdb;
        $sql = "SELECT COUNT(*) FROM " . self::$tables->node . " WHERE Hash = %s";
        $bool = $wpdb->get_var($wpdb->prepare($sql, $hash));
        if ($bool == "1" || $bool == 1) return true;
        return false;
    }

    public static function capture_redirect($wp) {
        $action = $trailobj = false;
        $trailhash = $trail = null;
        //trigger_error("query vars: " . print_r($wp->query_vars, true));
        if (isset($wp->query_vars['trail_slug'])) {
            $slug = $wp->query_vars['trail_slug'];
            if (!empty($slug) && !is_null($slug)) {
                self::redirectIfOldSlug($slug);
                if (isset($wp->query_vars['wt_action'])) {
                    $wt_action = $wp->query_vars['wt_action'];
                    $action = array_search($wt_action, self::$redirect_query_strings);
                    if ($action !== false) {
                        $trailhash = self::getNodeHashFromSlug($slug);
                        $trail = self::getNode($trailhash);
                        $trailobj = is_object($trail);
                    }
                }
            }
        }
        foreach (self::$redirect_query_strings as $key => $str) {
            if (isset($_GET[$str])) {
                $action = $key;
                $slug = $_GET[$str];
                $trailhash = self::getNodeHashFromID($_GET[$str]);
                $trail = self::getNode($trailhash);
                $trailobj = is_object($trail);
                break;
            }
        }
        if (false !== $action) {
            switch($action) {
                case "save":
                    if ($trailobj) {
                        $trail->save();
                    }
                    wp_redirect($trail->displayHREF());
                    exit;
                case "xml":
                    if ($slug == "full-site") {
                        header("Content-type: text/xml");
                        echo self::getPushXML();
                        exit;
                    } elseif ($trailobj) {
                        header("Content-type: text/xml");
                        list($dom, $root) = WordTrailsUtilities::prepareForXML();
                        $root->appendChild($trail->writeToXML($dom));
                        $dom->formatOutput = true;
                        echo $dom->saveXML();
                        exit;
                    } else {
                        wp_die("Invalid Trail specified", "Trail XML Generation");
                    }
                case "print":
                    self::printTrail($trailhash);
                    exit;
                case "ext":
                    if ($trailobj) $trail->display();
                    break;
                case "tv_save":
                    if (!empty($_POST['xml'])) self::initFromXML($_POST['xml']);
                    if (!empty($_POST['auto_save'])) exit;
                    $url = get_option('home');
                    if (!empty($_POST['redirect_after_save']))
                        $url = urldecode($_POST['redirect_after_save']);
                    wp_redirect($url);
                    exit;
                case "revert":
                    if ($trailobj) {
                        $trail->revertToSaved();
                    }
                    wp_redirect($trail->displayHREF());
                    exit;
            }
        }
    }

    public static function getNodeIDFromHash($hash) {
        if (is_array($hash)) {
            $ids = array();
            foreach ($hash as $h) {
                $ids[$h] = self::getNodeIDFromHash($h, $temp);
            }
            return $ids;
        }
        if (is_numeric($hash)) return $hash;
        self::makeInstance();
        if (isset(self::$instance->nodeHashToID[$hash]))
            return self::$instance->nodeHashToID[$hash];
        $node = self::getNode($hash, false);
        if (is_object($node)) {
            $nid = $node->getNodeID();
            if (!empty($nid)) {
                self::$instance->nodeIDToHash[$nid] = $hash;
                self::$instance->nodeHashToID[$hash] = $nid;
                return $nid;
            }
        }
        global $wpdb;
        $sql = "SELECT NodeID FROM " . self::$tables->node . " WHERE Hash = %s";
        $id = $wpdb->get_var($wpdb->prepare($sql, $hash));
        self::$instance->nodeIDToHash[$id] = $hash;
        self::$instance->nodeHashToID[$hash] = $id;
        return $id;
    }
    
    public static function makeTempNodeID($hash) {
        $id = "t-" . self::$instance->tempNodeCount++;
        self::$instance->nodeIDToHash[$id] = $hash;
        self::$instance->nodeHashToID[$hash] = $id;
        return $id;
    }

    public static function getNodeHashFromID($id) {
        if (is_array($id)) {
            $hashes = array();
            foreach ($id as $i) {
                $hashes[$i] = self::getNodeHashFromID($i);
            }
            return $hashes;
        }
        global $wpdb;
        self::makeInstance();
        if (!isset(self::$instance->nodeIDToHash[$id])) {
            if (!is_numeric($id)) {
                //trigger_error("not numeric id! $id");
                return $id;
            }
            $sql = "SELECT Hash FROM " . self::$tables->node . " WHERE NodeID = %d";
            self::$instance->nodeIDToHash[$id] = $wpdb->get_var($wpdb->prepare($sql, $id));
            self::$instance->nodeHashToID[self::$instance->nodeIDToHash[$id]] = $id;
        }
        return self::$instance->nodeIDToHash[$id];
    }
    
    public static function getNodeHashFromSlug($slug) {
        if (is_array($slug)) {
            $slugs = array();
            foreach($slug as $s) {
                $slugs[$s] = self::getNodeHashFromSlug($s);
            }
            return $slugs;
        }
        global $wpdb;
        self::makeInstance();
        if (isset(self::$instance->nodeSlugToHash[$slug])) return self::$instance->nodeSlugToHash[$slug];
        foreach(self::$instance->nodes as $hash => &$node) {
            if (WordTrailsUtilities::is_serialized_trail($node)) $node = unserialize($node);
            if (is_object($node)) {
                if (strtolower($node->getType()) != "trail") continue;
                $_slug = $node->getSlug();
                if ($slug == $_slug) {
                    self::$instance->nodeSlugToHash[$slug] = $hash;
                    return $hash;
                }
            }
        }
        $sql = "SELECT Slug, Hash FROM " . self::$tables->node . " WHERE Slug LIKE %s";
        $results = $wpdb->get_results($wpdb->prepare($sql, $slug), OBJECT);
        foreach ($results as $result) {
            if ($result->Slug == $slug) {
                self::$instance->nodeSlugToHash[$slug] = $hash;
                return $result->Hash;
            }
        }
        $sql = "SELECT Slug, Hash FROM " . self::$tables->old_slug . " WHERE Slug LIKE %s";
        $results = $wpdb->get_results($wpdb->prepare($sql, $slug), OBJECT);
        foreach ($results as $result) {
            if ($result->Slug == $slug) return $result->Hash;
        }
        return false;
    }
    public static function redirectIfOldSlug($slug) {
        global $wpdb;
        self::makeInstance();
        //trigger_error("checking if old slug: $slug");
        foreach(self::$instance->nodes as $hash => &$node) {
            //trigger_error(WordTrailsUtilities::is_serialized_trail($node) . ":" . $node);
            if (WordTrailsUtilities::is_serialized_trail($node)) $node = unserialize($node);
            if (is_object($node)) {
                if (strtolower($node->getType()) != "trail") continue;
                $_slug = $node->getSlug();
                //trigger_error("found slug: $_slug");
                if ($slug == $_slug) {
                    //trigger_error("storing $_slug as $hash");
                    self::$instance->nodeSlugToHash[$slug] = $hash;
                    return;
                }
            }
        }
        $sql = "SELECT Slug, Hash FROM " . self::$tables->node . " WHERE Slug LIKE %s";
        $results = $wpdb->get_results($wpdb->prepare($sql, $slug), OBJECT);
        //trigger_error("current slugs: " . print_r($results, true));
        foreach ($results as $result) {
            if ($result->Slug == $slug) {
                self::$instance->nodeSlugToHash[$slug] = $result->Hash;
                return;
            }
        }
        $sql = "SELECT Slug, Hash FROM " . self::$tables->old_slug . " WHERE Slug LIKE %s";
        $results = $wpdb->get_results($wpdb->prepare($sql, $slug), OBJECT);
        foreach ($results as $result) {
            if ($result->Slug == $slug) {
                $trail = self::getNode($result->Hash);
                if (is_object($trail)) {
                    $loc = preg_replace("/\/$slug\/?$/", user_trailingslashit("/".$trail->getSlug()), $_SERVER['REQUEST_URI']);
                    wp_redirect($loc, 301);
                    exit;
                }
            }
        }
        return false;
    }

    public static function buildNodeFromXML(DOMElement $XMLNode) {
        $type = $XMLNode->getAttribute("type");
        $hash = $XMLNode->getAttribute("hash");
        $node = self::getNode($hash, true);
        if (is_object($node)) {
            $node->initFromXML($XMLNode);
            return 1;
        }
        $sources = $XMLNode->getElementsByTagName("sorce_site");
        if ($sources->length <= 0) return false;
        if (!$sources->item(0)->hasChildNodes()) return false;
        $source = $sources->item(0)->firstChild->nodeValue;
        $isInternal = (get_option("home") == $source);
        $classes = self::$node_classes["external"];
        $id = null;
        if ($isInternal) {
            $classes = self::$node_classes["internal"];
            $ids = $XMLNode->getElementsByTagName("id");
            if ($ids->length > 0) {
                if ($ids->item(0)->hasChildNodes()) {
                    if (is_numeric($ids->item(0)->firstChild->nodeValue)) {
                        $id = (int)$ids->item(0)->firstChild->nodeValue;
                    }
                }
            }
        }
        if (false === array_search($type, array_keys($classes))) return false;
        require_once $classes[$type] . ".inc";
        self::$instance->nodes[$hash] = new $classes[$type]($hash, $id);
        self::$instance->nodes[$hash]->initFromXML($XMLNode);
        return true;
    }
    
    public static function updateNodeFromChangesetXML(DOMElement &$XMLNode) {
        $type = $XMLNode->getAttribute("type");
        $hash = $XMLNode->getAttribute("hash");
        $node = self::getNode($hash, true);
        if (is_object($node)) return $node->updateFromChangesetXML($XMLNode);
        return false;
    }

    public static function initFromXML($xmlstr) {
        $dom = new DOMDocument("1.0");
        $dom->preserveWhiteSpace = false;
	if ($dom->loadXML(stripcslashes($xmlstr)) !== false) {
	    $dom->formatOutput = true;
	    if ($dom->firstChild->firstChild->nodeName == "node" && strtolower($dom->firstChild->firstChild->getAttribute("type")) == "trail") {
                if ($dom->firstChild->nodeName == "trailchangeset") {
                    $ret = self::updateNodeFromChangesetXML($dom->firstChild->firstChild);
                } else {
        	    $ret = self::buildNodeFromXML($dom->firstChild->firstChild);
                }
                if (!empty($_POST["auto_save"])) {
                    die((string)$ret);
                }
	    }
	}
    }

    public static function &buildNodeFromDB($hashorid) {
        if (is_array($hashorid)) {
            $ret = array();
            foreach ($hashorid as $hori) {
                $ret[$hori] = &self::buildNodeFromDB($hori);
            }
            return $ret;
        } else {
            global $wpdb;
            $sql = "SELECT NodeID, Hash, Type, isInternal FROM " . self::$tables->node . " WHERE ";
            if (is_numeric($hashorid)) {
                $sql .= "NodeID = %d";
            } else {
                $sql .= "Hash = %s";
            }
            $nodeinfo = $wpdb->get_row($wpdb->prepare($sql, $hashorid), OBJECT);
            if (!is_object($nodeinfo)) return false;
            $classes = self::$node_classes["external"];
            if ($nodeinfo->isInternal) $classes = self::$node_classes["internal"];
            if (false === array_search(strtolower($nodeinfo->Type), array_keys($classes))) return false;
            require_once $classes[$nodeinfo->Type] . ".inc";
            self::makeInstance();
            self::$instance->nodes[$nodeinfo->Hash] = new $classes[$nodeinfo->Type]($nodeinfo->Hash, $nodeinfo->NodeID);
            return self::$instance->nodes[$nodeinfo->Hash];
        }
    }

    public static function getAllTrailNodes($additional = null) {
        global $wpdb;
        self::makeInstance();
        $sql = "SELECT NodeID, Hash, isInternal";
        if (!is_null($additional)) {
            if (is_array($additional)) {
                $additional = implode(", ", $additional);
            } else if (!is_string($additional)) {
                $additional = "";
            }
            $sql .= ", " . $additional;
        }
        $sql .= " FROM " . self::$tables->node . " WHERE Type = %s";
        $livenodes = array_filter(self::$instance->nodes, array("WordTrailsUtilities", "filter_unsaved_only"));
        if (!empty($livenodes))
            $sql .= " AND Hash NOT IN (" . implode(",", array_map(array("WordTrailsUtilities", "wrap_with_quotes"), array_keys($livenodes))) . ")";
        $trails["db"] = $wpdb->get_results($wpdb->prepare($sql, "trail"), OBJECT);
        $trails["live"] = array();
        foreach ($livenodes as $hash => &$node_or_str) {
            if (is_object($node_or_str)) {
                if ($node_or_str->getType() == "trail")
                    $trails["live"][] = &$node_or_str;
            }
            if (WordTrailsUtilities::is_serialized_trail($node_or_str))
                $trails["live"][] = self::getNode($hash);
        }
        return $trails;
    }

    public static function getAllTrailHashes() {
        global $wpdb;
        self::makeInstance();
        self::$instance->nodes = array_filter((array)self::$instance->nodes, array("WordTrailsUtilities", "filter_unsaved_only"));
        $sql = "SELECT Hash FROM " . self::$tables->node . " WHERE Type = %s ORDER BY Created DESC";
        //if (!empty(self::$instance->nodes))
            //$sql .= " AND Hash NOT IN (" . implode(",", array_map(array("WordTrailsUtilities", "wrap_with_quotes"), array_keys(self::$instance->nodes))) . ")";
        $trails = $wpdb->get_col($wpdb->prepare($sql, "trail"));
        foreach (self::$instance->nodes as $hash => $node_or_str) {
            if (is_object($node_or_str)) {
                if ("trail" == $node_or_str->getType())
                    array_push($trails, $hash);
            }
            if (WordTrailsUtilities::is_serialized_trail($node_or_str))
                array_push($trails, $hash);
        }
        $trails = array_unique($trails);
        usort($trails, array("WordTrailsUtilities", "sort_trails"));
        return $trails;
    }

    public static function getNodeNameFromHash($hash) {
        if (is_array($hash)) {
            $names = array();
            foreach ($hash as $h) {
                $names[$h] = self::getNodeNameFromHash($h);
            }
            return $names;
        }
        if (!WordTrailsUtilities::isHash($hash)) return null;
        $node = self::getNode($hash, false);
        if (is_object($node)) return $node->getName();
        if ($hash == self::get_option(self::SITE_HASH_OPTION)) return "<i>{Trail Index}</i>";
        $sql = "SELECT Name FROM " . self::$tables->node . " WHERE Hash = %s";
        global $wpdb;
        $name = $wpdb->get_var($wpdb->prepare($sql, $hash));
        if (empty($name)) $name = "<i>[Could not find node - has it been deleted?]</i>";
        return $name;
    }

    public static function getTrailSyncDates() {
        global $wpdb;
        $sql = "SELECT p.Stamp, n.Hash FROM " . self::$tables->push . " as p LEFT JOIN " . self::$tables->node . " as n ON p.TrailID = n.NodeID";
        $results = $wpdb->get_results($sql, OBJECT);
        $dates = array();
        foreach ($results as $result) {
            $dates[$result->Hash] = $result->Stamp;
        }
        return $dates;
    }

    public static function trailMemePush() {
        $hash = null;
        $hashes = null;
        if (isset($_REQUEST['hash']) && !empty($_REQUEST['hash'])) {
            $hash = $_REQUEST['hash'];
            if (strpos($hash, ","))
                $hashes = array_map(explode(",", $hash), trim);
            else
                $hashes = array($hash);
        }
        $xml = self::getPushXML($hashes);
        if (is_null($hashes)) $hashes = self::getAllTrailHashes();
        $sha = hash_init("sha1");
        hash_update($sha, $xml);
        hash_update($sha, trim(self::get_option(self::TRAILMEME_KEY_OPTION)));
        $sig = hash_final($sha);
        //$sig = "c000b2c152134fb01201ec6c0dedd429b12fa5c1";
        require_once trailingslashit(ABSPATH . WPINC) . 'class-snoopy.php';
        global $wp_version;
        $push = new Snoopy();
        $push->agent = 'WordPress/' . $wp_version;
        $push->read_timeout = 10;
        //$push->_submit_type = "multipart/form-data";
        $url = "http://trailmeme.com/import/";
        $req = array();
        $req['version'] = "WordTrails ".self::VERSION;
        $req['xml'] = $xml;
        $req['key'] = $sig;
        //echo "alert(unescape('".urlencode($xml)."'));";
        $report = error_reporting(0);
        try {
        if ($push->submittext($url, $req)) {
            $results = split("/",$push->results);
            $result = str_replace("+", " ", $results[count($results)-1]);
            if ($result == "Data successfully queued for import.") {
                global $wpdb;
                foreach ($hashes as $hash) {
                    $sql = "INSERT INTO " . self::$tables->push . " (PushID, TrailID, Stamp) VALUES(NULL, (SELECT NodeID FROM " . self::$tables->node . " WHERE Hash = %s), NOW()) ON DUPLICATE KEY UPDATE PushID=LAST_INSERT_ID(PushID), Stamp = NOW()";
                    $wpdb->query($wpdb->prepare($sql, $hash));
                }
            }
            if ($result == "") $result = "No Response.";
            foreach ($hashes as $hash) {
                echo "pushFinished('".$hash."', '".htmlspecialchars($result)."', '" . WordTrailsUtilities::stampToAbbr(date("Y-m-d H:i:s")) ."');";
            }
            die();
        } else {
            foreach ($hashes as $hash) {
                echo "pushError('Could not connect to TrailMeme','".$hash."');";
            }
            die();
        }
        } catch(Exception $e) {
            foreach ($hashes as $hash) {
                echo "pushError('Could not connect to TrailMeme','".$hash."');";
            }
            die();
        }
        error_reporting($report);
        //die("pushTrail('".$hash."','http://oakhill.wrc.xerox.com:5001/import','WordTrails ".$this->version."','".urlencode($xml)."','".$sig."')");
    }

    public static function getPushXML($hashes = null) {
        $dom = new DOMDocument("1.0");
        $root = $dom->createElement("traildoc");
        $root->setAttribute("application", "WordPress");
        $root->setAttribute("version", self::XMLVERSION);
        if ($hashes == null)
            $root->setAttribute("push", "full");
        else
            $root->setAttribute("push", "partial");
        $site = $dom->createElement("node");
        $site->setAttribute("type", "site");
        $site->setAttribute("hash", self::get_option(self::SITE_HASH_OPTION));
        $site->appendChild(WordTrailsUtilities::createBasicXMLNode("name", get_option("blogname"), $dom));
        $site->appendChild(WordTrailsUtilities::createBasicXMLNode("short_desc", get_option("blogdescription"), $dom));
        $site->appendChild(WordTrailsUtilities::createBasicXMLNode("source_url", get_option("home"), $dom));
        $children = $dom->createElement("children");
        if (is_null($hashes))
            $hashes = self::getAllTrailHashes();
        if (is_string($hashes)) {
            if (strpos($hashes, ","))
                $hashes = array_map(explode(",", $hashes), trim);
            if (!is_array($hashes))
                $hashes = array($hashes);
        }
        foreach ($hashes as $hash) {
            $trail = self::getNode($hash, true);
            if (!is_object($trail)) continue;
            if ($trail->getType() != "trail") continue;
            $trail->forceDBUpdate();
            $children->appendChild($trail->writeToXML($dom));
        }
        $site->appendChild($children);
        $root->appendChild($site);
        $dom->appendChild($root);
        //$dom->formatOutput = true;
	//$dom->normalizeDocument();
        //$dom->formatOutput = false;
	//$dom->preserveWhiteSpace = true; //false;
        return $dom->saveXML($root);
    }

    public static function sendAnalyticsToTrailMeme() {
        if (is_null(self::get_option(self::TRAILMEME_KEY_OPTION))) return false;
        $analytics = WordTrailsAnalytics::pullAnalyticsToTransmit();
        $full_data = "";
        if (is_array($analytics["print"])) {
            foreach ($analytics["print"] as $print) {
                $full_data .= "pdf,{$print->TrailHash},".strtotime($print->Stamp).",{$print->SessionID},{$print->ButtonID},{$print->NodeCount},{$print->PageCount},{$print->SectionCount}\r";
            }
        }
        if (is_array($analytics["usage"])) {
            foreach ($analytics["usage"] as $usage) {
                $full_data .= "flw,{$usage->TrailHash},{$usage->NodeHash},{$usage->Stamp},{$usage->SessionID},{$usage->Type},{$usage->How}\r";
            }
        }
        $full_data = trim($full_data);
        if ($full_data == "") return true;
        $sha = hash_init("sha1");
        hash_update($sha, $full_data);
        hash_update($sha, trim(self::get_option(self::TRAILMEME_KEY_OPTION)));
        $sig = trim(hash_final($sha));
        require_once trailingslashit(ABSPATH . WPINC) . '/class-snoopy.php';
        global $wp_version;
        $push = new Snoopy();
        $push->agent = 'WordPress/' . $wp_version;
        $push->read_timeout = 2;
        //$push->_submit_type = "multipart/form-data";
        $url = "http://trailmeme.com/site_uses/";
        $req = array();
        $req['url'] = get_option("home");
        $req['data'] = $full_data;
        $req['key'] = $sig;
        //echo "alert(unescape('".urlencode($xml)."'));";
        $report = error_reporting(0);
        try {
        if ($push->submittext($url, $req)) {
            $results = split("/",$push->results);
            $result = str_replace("+", " ", $results[count($results)-1]);
            if (substr($result, 0, 1) == "1") {
                global $wpdb;
                if (is_array($analytics["print"])) {
                    $sql = "INSERT INTO " . self::$tables->print_push . " (PushID, PrintID, Stamp) VALUES ";
                    $inserts = array();
                    foreach ($analytics["print"] as $print) {
                        $inserts[] = "(NULL, {$print->PrintID}, NULL)";
                    }
                    $sql .= implode(", ", $inserts);
                    if (!empty($inserts)) $wpdb->query($sql);
                }
                if (is_array($analytics["usage"])) {
                    $sql = "INSERT INTO " . self::$tables->usage_push . " (PushID, UsageID, Stamp) VALUES ";
                    $inserts = array();
                    foreach ($analytics["usage"] as $usage) {
                        $inserts[] = "(NULL, {$print->UsageID}, NULL)";
                    }
                    $sql .= implode(", ", $inserts);
                    if (!empty($inserts)) $wpdb->query($sql);
                }
            } else {
                error_log("TrailMeme reports error: $result");
            }
        } else {
            error_log("connection to trailmeme failed");
        }
        } catch(Exception $e) {
            error_log("connection to trailmeme failed");
        }
        error_reporting($report);
    }

    public static function &createNewNode($internal=true, $type="node") {
        $classes = self::$node_classes[($internal ? "internal" : "external")];
        if (false === array_search($type, array_keys($classes))) return false;
        require_once $classes[$type] . ".inc";
        $hash = WordTrailsUtilities::newHash();
        self::makeInstance();
        self::$instance->nodes[$hash] =  new $classes[$type]($hash);
        return self::$instance->nodes[$hash];
    }

    public static function &getNode($hash, $make_if_not_exist = true) {
        if (is_array($hash)) {
            $ret = array();
            foreach ($hash as $h) {
                $ret[$h] = self::getNode($h, $make_if_not_exist);
            }
            return $ret;
        }
        self::makeInstance();
        if (isset(self::$instance->nodes[$hash])) {
            self::$instance->nodes[$hash] = maybe_unserialize(self::$instance->nodes[$hash]);
            if (is_object(self::$instance->nodes[$hash]))
                return self::$instance->nodes[$hash];
        }
        $n = null;
        if (!$make_if_not_exist) return $n;
        return self::buildNodeFromDB($hash);
    }

    public static function getNodeInfo($hashes, $details = null) {
        if (is_null($hashes) || empty($hashes)) return false;
        if (!is_array($hashes)) $hashes = array($hashes);
        if (!is_array($details)) $details = array();
        $details = array_map(strtolower, array_unique(array_merge($details, array("name", "href"))));
        $results = array();
        $from_db = array();
        foreach ($hashes as $hash) {
            $info = array();
            $node = self::getNode($hash);
            if (is_object($node)) {
                foreach ($details as $detail) {
                    $inf = null;
                    switch($detail) {
                        case "name":
                            $inf = $node->getName();
                            break;
                        case "href":
                            $inf = $node->displayHREF();
                            break;
                    }
                    $info[$detail] = $inf;
                }
            } else {
                array_push($from_db, $hash);
            }
            $results[$hash] = $info;
        }
        if (!empty($from_db)) {
            global $wpdb;
            $sql = "SELECT n.Hash, n.Type";
            foreach ($details as $detail) {
                switch($detail) {
                    case "name":
                        $sql .= ", n.Name as $detail";
                        break;
                    case "href":
                        $sql .= ", n.ReferenceID as $detail";
                        break;
                }
            }
            $sql .= " FROM " . self::$tables->node . " as n WHERE n.Hash IN (" . implode(",", array_map(array("WordTrailsUtilities", "wrap_with_quotes"), $from_db)) . ")";
            $infos = $wpdb->get_results($sql, OBJECT);
            if (is_array($infos) && !empty($infos)) {
                foreach ($infos as $result) {
                    $info = array();
                    foreach ($details as $detail) {
                        $inf = null;
                        switch ($detail) {
                            case "href":
                                if ($result->Type == "node" && isset($result->$detail) && !is_null($result->$detail))
                                    $inf = WordTrailsUtilities::permalink($result->$detail);
                                break;
                            default:
                                $inf = $result->$detail;
                                break;
                        }
                        if (!is_null($inf))
                            $info[$detail] = $inf;
                    }
                    $results[$result->Hash] = $info;
                }
            }
        }
        return $results;
    }

    public static function storeUnsavedChild($parent, $child) {
        self::makeInstance();
        if (!is_array(self::$instance->unsaved_parents[$child])) self::$instance->unsaved_parents[$child] = array();
        array_push(self::$instance->unsaved_parents[$child], $parent);
        self::$instance->unsaved_parents[$child] = array_unique(self::$instance->unsaved_parents[$child]);
    }
    public static function removeUnsavedChild($parent, $child) {
        self::makeInstance();
        if (!is_array(self::$instance->unsaved_parents[$child])) return 0;
        self::$instance->unsaved_parents[$child] = array_diff(self::$instance->unsaved_parents[$child], array($parent));
        if (empty(self::$instance->unsaved_parents[$child])) unset(self::$instance->unsaved_parents[$child]);
    }

    public static function unregisterNode($hash) {
        self::makeInstance();
        $purge_from = array_keys(self::$instance->nodes);
        $delete_me = self::getNode($hash);
        if (is_object($delete_me) && $delete_me->getParentIDs(true)) {
            $purge_from = array_unique(array_merge($purge_from, $delete_me->parent_hashes));
        }
        foreach ($purge_from as $h) {
            if ($h == $hash) continue;
            $node = self::getNode($h);
            $node->removeChild($hash);
        }
        if (isset(self::$instance->nodeHashToID[$hash])) {
            if (isset(self::$instance->nodeIDToHash[self::$instance->nodeHashToID[$hash]])) {
                unset(self::$instance->nodeIDToHash[self::$instance->nodeHashToID[$hash]]);
            }
            unset(self::$instance->nodeHashToID[$hash]);
        }
        unset(self::$instance->nodes[$hash]);
    }

    public static function hashesFromPID($pid) {
        global $wpdb;
        if (!is_numeric($pid)) return false;
        $pid = (int)$pid;
        self::makeInstance();
        $sql = "SELECT Hash FROM " . self::$tables->node . " WHERE isInternal = 1 AND Type = \"node\" AND ReferenceID = %d";
        $hashes = $wpdb->get_col($wpdb->prepare($sql, $pid));
        $merge = array();
        $diff = array();
        foreach (array_keys(self::$instance->nodes) as $hash) {
            $node = self::$instance->nodes[$hash];
            if (is_object($node)) {
                if (isset($node->PostID)) {
                    if ($node->PostID == $pid) {
                        if ($node->delete_on_save) {
                            $diff[] = $hash;
                        } else {
                            $merge[] = $hash;
                        }
                    }
                }
            } elseif(is_serialized($node)) {
                if (false !== strpos($node, 'O:18:"WordTrailsPostNode"')) {
                    if (false !== strpos($node, '{s:6:"PostID";i:' . $pid)) {
                        if (false !== strpos($node, 's:14:"delete_on_save";b:1;'))
                            $diff[] = $hash;
                        else
                            $merge[] = $hash;
                    }
                }
            }
        }
        $hashes = array_unique(array_diff(array_merge($hashes,$merge), $diff));
        return $hashes;
    }

    public static function editFormBox($args) {
        global $post, $wpdb;
        $pid = (int)$post->ID;
        if (0 == $pid) {
            echo "<i>This post must be saved before it can be added to a trail.</i>";
            return false;
        }
        $hashes = self::hashesFromPID($pid);

        $trails = self::getAllTrailNodes("Name");

        $on_trails = array();
        $not_on_trails = array();
        foreach ($trails["live"] as &$trail) {
            if (count(array_intersect($hashes, $trail->child_hashes)) > 0) $on_trails[] = $trail->getHash();
        }
        self::makeInstance();
        $sql = "SELECT n.Hash FROM " . self::$tables->node . " as n LEFT JOIN " . self::$tables->node_rel . " as nr ON n.NodeID = nr.ChildID WHERE nr.ParentID = %d";
        foreach ($trails["db"] as &$trail) {
            $children = $wpdb->get_col($wpdb->prepare($sql, $trail->NodeID));
            if (count(array_intersect($hashes, $children)) > 0) $on_trails[] = $trail->Hash;
        }

        ?>
        <input type="hidden" name="pid" value="<?php echo $pid; ?>" />
        <input type="hidden" name="trl_blz" value="1" />
        <script language="JavaScript" type="text/javascript" src="<?php echo self::$urlpath; ?>js/trailBlazing.js"></script>
        <div style="float:left;">
        <strong>Add this post to:</strong><br />
        <select id="blaze_select" name="blaze_trail" onkeyup="blazeSelect();" onchange="blazeSelect();">
            <option<?php if (!$blazing_default) echo ' selected="selected"'; ?> value="null" style="padding-bottom:5px">-----Select a Trail-----</option><?php
            foreach ($trails["live"] as $trail) {
                if (false !== array_search($trail->getHash(), $on_trails)) continue; ?>
    
            <option value="<?php echo $trail->getHash(); ?>"<?php if ($blazing_default && $trail->getNodeID() == WordTrailsGlobal::$instance->blazing_trail_id) echo ' selected="selected"'; ?>><?php echo $trail->getName(); ?></option><?php
            }
            foreach ($trails["db"] as $trail) {
                if (false !== array_search($trail->Hash, $on_trails)) continue; ?>
    
            <option value="<?php echo $trail->Hash; ?>"<?php if ($blazing_default && $trail->NodeID == WordTrailsGlobal::$instance->blazing_trail_id) echo ' selected="selected"'; ?>><?php echo $trail->Name; ?></option><?php
            }?>
    
            <option value="new_trail" style="padding-top:5px">Make New Trail &rarr;</option>
        </select>
        <fieldset id="new_child_options" style="border: none;" class="hidden">
            <label><input type="radio" name="new_child_radio" value="as_child"<?php if (!self::$instance->blazing_default_norel) echo ' checked="checked"'; ?> /> Add as child of last node added</label><br />
            <label><input type="radio" name="new_child_radio" value="no_rels"<?php if (self::$instance->blazing_default_norel) echo ' checked="checked"'; ?> /> Add with no relationships</label>
        </fieldset>
        <fieldset id="new_trail_options" style="border: none;" class="hidden">
            <label>New Trail Name:<br/><input type="text" name="new_trail_name" value=""></label>
        </fieldset>
        <input type="submit" id="blaze_submit" value="Add Post" class="<?php wt_button_class(); ?> hidden" />
        </div><?php if (count($on_trails) > 0) { ?>
        <div style="float: left;padding-left: 20px;">
        <input type="hidden" id="remove_from_trail" name="remove_from_trail" value="false" />
        <strong>Remove post from:</strong><br />
        <select id="blaze_remove_select" name="remove_trail" onkeyup="removeSelect();" onchange="removeSelect();">
            <option selected="selected" name="null" value="null">-----Select a Trail-----</option>
            <optgroup title="trails" style="padding-top:5px">
            </optgroup><?php
        foreach ($trails["live"] as &$trail) {
            if (false !== array_search($trail->getHash(), $on_trails)) { ?>

                <option name="<?php echo $trail->getHash(); ?>" value="<?php echo $trail->getHash(); ?>"><?php echo $trail->getName(); ?></option><?php
            }
        }
        foreach ($trails["db"] as &$trail) {
            if (false !== array_search("$trail->Hash", $on_trails)) { ?>

                <option name="<?php echo $trail->Hash; ?>" value="<?php echo $trail->Hash; ?>"><?php echo $trail->Name; ?></option><?php
            }
        } ?>

        </select>
        <input type="submit" id="remove_submit" value="Remove Post" class="<?php wt_button_class(); ?> hidden" onmousedown="jQuery('#remove_from_trail').attr('value', 'true');" />
        </div>
        <?php } ?>
        <br class="clear" /><?php
    }
    public static function deletePID($pid) {
        if ($pid == self::get_option(self::TRAILS_PAGE_OPTION)) {
	    self::update_option(self::TRAILS_PAGE_OPTION, null);
            return;
        }
        if ($pid == self::get_option(self::PREV_TRAILS_PAGE_OPTION)) {
	    self::update_option(self::PREV_TRAILS_PAGE_OPTION, null);
            return;
        }
        $hashes = self::hashesFromPID($pid);
        if (empty($hashes)) return false;
        foreach ($hashes as $hash) {
            $node = self::getNode($hash, true);
            if (!is_object($node)) continue;
            $node->delete(true);
        }
    }
    
    public static function trashedPID($pid) {
        if ($pid == self::get_option(self::TRAILS_PAGE_OPTION)) {
	    self::update_option(self::TRAILS_PAGE_OPTION, null);
	    self::update_option(self::PREV_TRAILS_PAGE_OPTION, $pid);
        }
    }
    public static function untrashedPID($pid) {
        if (is_int(self::get_option(self::TRAILS_PAGE_OPTION))) return;
        if ($pid == self::get_option(self::PREV_TRAILS_PAGE_OPTION)) {
	    self::update_option(self::PREV_TRAILS_PAGE_OPTION, null);
	    self::update_option(self::TRAILS_PAGE_OPTION, $pid);
        }
    }

    public function blazeTrail() {
        $pid = $_POST['pid'];
        if (!is_numeric($pid)) return false;
        $pid = (int)$pid;
        $tid = null;
        if ($_POST['remove_from_trail'] == "true") {
            $remove_trail = $_POST['remove_trail'];
            if ($remove_trail == "null") return false;
            if (!WordTrailsUtilities::isHash($remove_trail)) return false;
            $trail = null;
            $trail = self::getNode($remove_trail);
            if (!is_object($trail)) return false;
            $hashes = self::hashesFromPID($pid);
            $delete = array_intersect($hashes, $trail->child_hashes);
            if (empty($delete)) return false;
            foreach ($delete as $del) {
                $node = self::getNode($del, true);
                if (!is_object($node)) continue;
                $node->delete(true);
            }
        } else {
            $blaze_trail = $_POST['blaze_trail'];
            if ($blaze_trail == "null") return false;
            if (!WordTrailsUtilities::isHash($blaze_trail) && $blaze_trail != "new_trail") return false;
            $trail = null;
            if ($blaze_trail == "new_trail") {
                $trail = self::createNewNode(true, "trail");
                $trail->setName($_POST['new_trail_name']);
            } else {
                $trail = self::getNode($blaze_trail);
            }
            if (!is_object($trail)) return false;
            $post = self::createNewNode(true);
            $post->setPostID($pid);
            $post->trail_hash = $trail->getHash();
            $trail->addChild($post->getHash());
            self::makeInstance();
            self::$instance->blazing_trail_id = $trail->getNodeID();
            if ($blaze_trail != "new_trail") {
                if ($_POST['new_child_radio'] == "as_child") {
                    self::$instance->blazing_default_norel = false;
                    $last_hash = $trail->getLastChild($post->getHash());
                    if (!is_null($last_hash)) {
                        $parent = self::getNode($last_hash, true);
                        if (is_object($parent)) {
                            $parent->addChild($post->getHash());
                        }
                    }
                } else {
                    self::$instance->blazing_default_norel = true;
                }
            }
            $tid = $trail->getNodeID();
        }
        if (!defined("WP_ADMIN") || WP_ADMIN !== TRUE) {
            $url = WordTrailsUtilities::permalink($pid, $tid);
            if (!headers_sent()) {
                wp_redirect($url);
                exit;
            } else { ?>
<meta http-equiv="refresh" content="0;url='<?php echo $url; ?>'" /><?php

            }
        }
    }

    public static function getCreationDate($hash, $chkdb = true) {
        if (is_array($hash)) {
            $sql = "SELECT Hash, Created FROM " . self::$tables->node . " WHERE Hash IN (" . implode(", ", array_map(array("WordTrailsUtilities", "wrap_with_quotes"), $hash)) . ")";
            global $wpdb;
            $hashes = array();
            if (empty($hash)) return $hashes;
            $dates = $wpdb->get_results($sql, OBJECT);
            foreach ($dates as $date) {
                $hashes[$date->Hash] = $date->Created;
            }
            foreach ($hash as $h) {
                $date = self::getCreationDate($h, false);
                if ($date !== false) {
                    if ($date != $hashes[$h] || !isset($hashes[$h])) {
                        $hashes[$h] = $date;
                    }
                }
            }
            return $hashes;
        }
        $node = self::getNode($hash, false);
        if (is_object($node)) return $node->getCreated();
        if (!$chkdb) return false;
        $sql = "SELECT Created FROM " . self::$tables->node . " WHERE Hash = %s";
        global $wpdb;
        return $wpdb->get_var($wpdb->prepare($sql, $hash));
    }

    public static function trackHits($pid) {
        if (self::get_option(self::ANALYTICS_COLLECT_OPTION) != 1) return false;
        if (!empty($_GET["t"])) $CurrentTrailHash = self::getNodeHashFromID($_GET["t"]);
        global $wp;
        if (isset($wp->query_vars['trail_slug'])) {
            $slug = $wp->query_vars['trail_slug'];
            if (!empty($slug) && !is_null($slug)) $CurrentTrailHash = self::getNodeHashFromSlug($slug);
        }
        if (is_page(self::get_option(self::TRAILS_PAGE_OPTION))) {
            if (isset($CurrentTrailHash)) {
                WordTrailsAnalytics::hit($CurrentTrailHash, null, WordTrailsAnalytics::BEV, WordTrailsAnalytics::DIRECT);
            } else {
                WordTrailsAnalytics::hit(self::get_option(self::SITE_HASH_OPTION), null, WordTrailsAnalytics::INDEX, WordTrailsAnalytics::DIRECT);
            }
        } else {
            $nodeHashes = (array)self::hashesFromPID($pid);
            foreach ($nodeHashes as $hash) {
                $node = self::getNode($hash, true);
                if (!is_object($node)) continue;
                if (!isset($node->trail_hash)) continue;
                if (isset($CurrentTrailHash)) {
                    if ($CurrentTrailHash == $node->trail_hash) {
                        WordTrailsAnalytics::hit($CurrentTrailHash, $hash, WordTrailsAnalytics::WEV, WordTrailsAnalytics::DIRECT);
                    } else {
                        WordTrailsAnalytics::hit($node->trail_hash, $hash, WordTrailsAnalytics::WEV, WordTrailsAnalytics::INDIRECT);
                    }
                } else {
                    WordTrailsAnalytics::hit($node->trail_hash, $hash, WordTrailsAnalytics::WEV, WordTrailsAnalytics::CHANCE);
                }
            }
        }
    }


    // {{{ printTrail($id)
    public static function printTrail($hashorid) {
        ob_start();
        require_once 'WordTrailsPDFExtender.inc';
        $trail = self::getNode(self::getNodeHashFromID($hashorid));
        if (!is_object($trail)) wp_die("Fatal error creating trail object.", "Print Trail");
        list($print_nodes, $sections) = $trail->gatherPrintNodes();
        /*
        global $wpdb;
        if (false === $this->makeTrailObj($id, "all", "post_info")) return false;
        $trail = $this->trails[$id];
        if (is_null($trail) || false === $trail)
            return false;
        $trail->print_prepare("mID");
        */
        $pdf=new WordTrailsPDFExtender();
        $pdf->AliasNbPages();
        $pdf->AddPage();
        $pdf->Title(WordTrailsUtilities::plain_quotes($trail->getName()));
        if (!is_null($trail->getShortDesc())){
            $pdf->ShortDesc(WordTrailsUtilities::plain_quotes($trail->getShortDesc()));
        }
        if (!is_null($trail->getLongDesc())) {
            $pdf->LongDesc(WordTrailsUtilities::plain_quotes($trail->getLongDesc()));
        }
        
        $links = $pdf->ContentLinks(array_map(create_function('$hash','return array("Hash" => $hash, "Name" => WordTrailsGlobal::getNode($hash)->getName());'), $print_nodes));
        foreach ($print_nodes as $hash) {
            $node = self::getNode($hash);
            if ($node->getType() == "node") {
                global $post, $more, $wpdb;
                $post = $wpdb->get_row($wpdb->prepare("SELECT * FROM $wpdb->posts WHERE ID = %d LIMIT 1", $node->getReferenceID()));
                setup_postdata($post);
                $more=1;
                $pdf->MarkerHead(WordTrailsUtilities::plain_quotes($node->getName()),$links[$hash],get_the_author(), the_date(null, null, null, false));
                if ($post->post_status == 'publish') {
                    $pdf->SetFont('Arial', '', 13);
                    $pdf->WriteHTML(WordTrailsUtilities::plain_quotes(get_the_content()));
                }
            } else {
                $pdf->MarkerHead(WordTrailsUtilities::plain_quotes($node->getName()), $links[$hash]);
                $pdf->SetFont('Arial', '', 13);
                $pdf->WriteHTML("Node type: " . $node->getType() . " not yet printable.");

            }
        }
        $bid = null;
        if (!empty($_GET['button'])) $bid = (int)$_GET['button'];
        if (self::get_option(self::ANALYTICS_COLLECT_OPTION) == 1)
            WordTrailsAnalytics::pdf($trail->getHash(), $bid, count($print_nodes), count($pdf->pages), $sections);
        ob_end_clean();
        $pdf->Output();
        exit;
    }
    // }}}


    public function __toString() {
        if (is_object($this))
            return "WordTrails Global object";
        else
            return "not really sure...";
    }

}
// }}}

//*
//error_reporting(E_ALL ^ E_NOTICE ^ E_STRICT); 
$old_error_handler = set_error_handler("userErrorHandler");

function userErrorHandler ($errno, $errmsg, $filename, $linenum,  $vars) {
    if ($errno == 2048) return false;
    $time=date("D M d H:i:s Y"); 
    // Get the error type from the error number 
    $errortype = array (1    => "Error",
                        2    => "Warning",
                        4    => "Parsing Error",
                        8    => "Notice",
                        16   => "Core Error",
                        32   => "Core Warning",
                        64   => "Compile Error",
                        128  => "Compile Warning",
                        256  => "User Error",
                        512  => "User Warning",
                        1024 => "User Notice",
                        2048 => "Strict",
                        4096 => "Deprecated",
                        8192 => "Recoverable",
                        16384 => "User Deprecated");
    $errlevel=$errortype[$errno];
    $filename = substr($filename, strlen(ABSPATH));
    if (strpos($filename, "wordtrails")) $filename = substr($filename, strlen("\wp-content\plugins\wordtrails\includes"));
    //Write error to log file (CSV format)
    if ($errno == 8) return;
    $errfile=fopen("C:\\Program Files (x86)\\Apache Software Foundation\\Apache2.2\\logs\\error.log","a");
    fputs($errfile,"[$time] [$filename:$linenum] ($errortype) $errmsg\r\n");
    fclose($errfile);
}
//*/
?>