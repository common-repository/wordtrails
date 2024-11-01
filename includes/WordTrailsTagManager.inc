<?php
/**
 * This is the Tag Manager class
 *
 * This class manages all of the tags used by all of the trails on this site.
 *
 * @package WordTrails
 * @since 0.2.0
 * @author Jesse Silverstein <jesse.silverstein@xerox.com>
 * @copyright 2008 XIG: SemPrint
 * @global WordPress $wpdb WordPress database management class
 */

// {{{ WordTrailsTagManager
/**
 * TagManager Class
 *
 * This class will store all of the tags used by all of the trails on this site.
 * Likely it will need a lot of refining for big sites.
 * This version loads all of the tags into an array.
 * It keeps track of new and temporary only tags, and then saves the new ones
 * on __destruct.
 *
 * @package WordTrails
 * @since 0.2.0
 * @author Jesse Silverstein <jesse.silverstein@xerox.com>
 * @copyright 2008 XIG: SemPrint
 * @global WordPress $wpdb WordPress database management class
 */

class WordTrailsTagManager {
    public $tags = array();
    /*
    public $newtags = array();
    public $temptags = array();

    protected $newcount = 0;
    protected $tempcount;
    */

    public function __construct() {
	$this->cleanupTags();
	$this->__wakeup();
    }
    /*
    public function __destruct() {
	$this->saveNewTag($this->newtags);
    }
    */
    public function __sleep() {
	return array("tags");
    }
    public function __wakeup(){
	global $wpdb;
	WordTrailsUtilities::setTables();
	$sql = "SELECT TagID, Tag FROM " . WordTrailsGlobal::$tables->tag;
	if (!empty($this->tags)) $sql .= " WHERE TagID NOT IN(" . implode(",", array_keys($this->tags)) . ")";
	$results = $wpdb->get_results($sql, OBJECT);
	foreach ($results as $tag) {
	    $this->tags[$tag->TagID] = $tag->Tag;
	}
    }
    protected function saveNewTag($tag) {
	global $wpdb;
	if (is_array($tag)) {
	    $ids = array();
	    foreach ($tag as $t) {
		$ids[] = $this->saveNewTag($t);
	    }
	    return $ids;
	}
	if (!is_string($tag) || is_null($tag) || empty($tag)) return false;
	$sql = "INSERT INTO " . WordTrailsGlobal::$tables->tag . " (TagID, Tag) VALUES(NULL, %s) ON DUPLICATE KEY UPDATE TagID = LAST_INSERT_ID(TagID)";
	$wpdb->query($wpdb->prepare($sql, $tag));
	$tid = (int)$wpdb->insert_id;
	if ($tid == 0) {
	    $findIDsql = "SELECT TagID FROM " . WordTrailsGlobal::$tables->tag . " WHERE Tag = %s";
	    $tid = (int)$wpdb->get_var($wpdb->prepare($findIDsql, $tag));
	}
	$this->tags[$tid] = $tag;
	return $tid;
    }

    public function addTag($tag) {
	if (is_array($tag)) {
	    $ids = array();
	    foreach ($tag as $t) {
		$ids[] = $this->addTag($t);
	    }
	    return $ids;
	}
	if (!is_string($tag) || is_null($tag)) return false;
	$tag = trim($tag);
	$tagsearch = array_search($tag, $this->tags);
	if (false !== $tagsearch) return (int)$tagsearch;

	return $this->saveNewTag($tag);
    }

    public function tag($id) {
	if (is_array($id)) {
	    $ids = array();
	    foreach ($id as $i) {
		$ids[] = $this->tag($i);
	    }
	    return $ids;
	}
	if (isset($this->tags[$id])) return $this->tags[$id];
	return false;
    }

    public function tid($tag) {
	if (is_array($tag)) {
	    $ids = array();
	    foreach ($tag as $t) {
		$ids[] = $this->tid($t);
	    }
	    return $ids;
	}
	error_log("retrieving tid from tag: " . $tag);
	return array_search($tag, $this->tags);
    }

    public function cleanupTags($force = false) {
	$last_clean = WordTrailsGlobal::get_option(WordTrailsGlobal::TAG_CLEANUP_OPTION);
	if (is_null($last_clean) || $last_clean + (5*24*60*60) < time() || $force) {
	    global $wpdb;
	    WordTrailsGlobal::update_option(WordTrailsGlobal::TAG_CLEANUP_OPTION, time());
	    $sql = "DELETE FROM " . WordTrailsGlobal::$tables->tag . " WHERE TagID NOT IN (SELECT TagID FROM " . WordTrailsGlobal::$tables->node_tag . ")";
	    $wpdb->query($sql);
	}
    }
}
// }}}
?>