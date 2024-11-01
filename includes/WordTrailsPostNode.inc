<?php
/**
 * This is the Post Node class - it extends the basic node class
 *
 * This file contains the WordTrailsPostNode class, the basic node that
 * represents a post on this blog.
 *
 * @package WordTrails
 * @since 0.2.0
 * @author Jesse Silverstein <jesse.silverstein@xerox.com>
 * @copyright 2008 XIG: SemPrint
 * @global WordPress $wpdb WordPress database management class
 */

require_once "WordTrailsNode.inc";

// {{{ WordTrailsPostNode
/**
 * Post Node class
 *
 * This is the Post node class. It is used to represent an internal node that is
 * a post on this blog. It extends the basic Node class.
 *
 * @package WordTrails
 * @since 0.2.0
 * @author Jesse Silverstein <jesse.silverstein@xerox.com>
 * @copyright 2008 XIG: SemPrint
 * @global WordPress $wpdb WordPress database management class
 */
class WordTrailsPostNode extends WordTrailsNode {
    // {{{ Properties
    public $PostID = null;
    protected $Type = "node";
    public $post_info = array();
    // }}}

    public function setPostID($id) {
	if (!is_int($id)) return false;
	$this->ReferenceID = $this->PostID = $id;
	$this->log("Assigned to Post ID: $id");
	$this->updatePostInfo();
    }

    // {{{ Database accessors
    // {{{ update()
    public function update() {
	$results = parent::update();
	$this->PostID = $this->ReferenceID;
	$this->updatePostInfo();
	return $results;
    }
    // }}}

    // {{{ updatePostInfo()
    protected function updatePostInfo() {
	global $wpdb;
	if (is_null($this->PostID)) return false;
        $sql = "SELECT p.post_title as Title, p.post_date as Date, u.display_name as Author FROM $wpdb->posts AS p LEFT JOIN $wpdb->users AS u ON p.post_author = u.ID WHERE p.ID = %d";
	$this->post_info = $wpdb->get_row($wpdb->prepare($sql, $this->PostID), OBJECT);
	if (empty($this->info->Name))
	    $this->info->Name = $this->post_info->Title;
        $sql = "SELECT trm.name AS Name, ttax.taxonomy AS Type FROM $wpdb->terms AS trm";
	$sql .=" LEFT JOIN $wpdb->term_taxonomy AS ttax ON trm.term_id = ttax.term_id";
	$sql .= " WHERE ttax.term_taxonomy_id IN";
	$sql .= " (SELECT tr.term_taxonomy_id FROM $wpdb->term_relationships AS tr WHERE tr.object_id = $this->PostID)";
        $terms = $wpdb->get_results($sql,OBJECT);
	$this->post_info->tags = array();
	$this->post_info->cats = array();
        foreach ($terms as $term) {
            if ($term->Type == "category") {
                array_push($this->post_info->cats, $term->Name);
            } else if ($term->Type == "post_tag") {
                array_push($this->post_info->tags, $term->Name);
            }
	}
    }
    // }}}
    // }}}

    // {{{ XML Functions
    // {{{ initFromXML(DOMElement &$XMLNode)
    public function initFromXML(DOMElement &$XMLNode) {
	parent::initFromXML($XMLNode);
	$this->PostID = $this->ReferenceID;
	$this->updatePostInfo();
    }
    // }}}

    // {{{ writeAuthorsToXML(&$dom)
    protected function writeAuthorsToXML(DOMDocument &$dom) {
	return WordTrailsUtilities::explodeToXMLNodes("authors", "author", array($this->post_info->Author), null, $dom);
    }
    // }}}

    // {{{ writeOtherInfoToXML(&$node)
    protected function writeOtherInfoToXML(DOMElement &$node, DOMDocument &$dom) {
	$node->appendChild(WordTrailsUtilities::explodeToXMLNodes("wp_tags", null, $this->post_info->tags, null, $dom));
	$node->appendChild(WordTrailsUtilities::explodeToXMLNodes("wp_categories", "category", $this->post_info->cats, null, $dom));
	return $node;
    }
    // }}}
    // }}}

    // {{{ displayHREF
    public function displayHREF() {
	if (is_null($this->PostID)) return false;
	return WordTrailsUtilities::permalink($this->PostID, $this->trail_hash);
    }
    // }}}

    // {{{ display()
    public function display() {
	wp_redirect($this->displayHREF());
    }
    // }}}

    public function __sleep() {
	$par = parent::__sleep();
	if (is_null($par)) return null;
	return array_unique(array_merge(array_keys(get_object_vars($this)),$par));
    }
}
// }}}
?>