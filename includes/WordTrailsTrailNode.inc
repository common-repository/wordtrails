<?php
/**
 * This is the Trail Node class - it extends the basic node class
 *
 * This file contains the WordTrailsTrailNode class, the basic node that
 * represents a trail on this blog.
 *
 * @package WordTrails
 * @since 0.2.0
 * @author Jesse Silverstein <jesse.silverstein@xerox.com>
 * @copyright 2008 XIG: SemPrint
 * @global WordPress $wpdb WordPress database management class
 */

require_once "WordTrailsNode.inc";

// {{{ WordTrailsTrailNode
/**
 * Trail Node class
 *
 * This is the Trail Node class. It is used to represent an internal node that is
 * a trail on this blog. It extends the basic Node class.
 *
 * @package WordTrails
 * @since 0.2.0
 * @author Jesse Silverstein <jesse.silverstein@xerox.com>
 * @copyright 2008 XIG: SemPrint
 * @global WordPress $wpdb WordPress database management class
 */

class WordTrailsTrailNode extends WordTrailsNode {
    // {{{ properties
    protected $Type = "trail";

    // }}}

    public function hasUnsaved() {
	if (parent::hasUnsaved()) return true;
	return $this->hasUnsavedChildren();
    }
    public function hasUnsavedChildren() {
	foreach ($this->child_hashes as $hash) {
	    $child = WordTrailsGlobal::getNode($hash, false);
	    if (is_object($child)) {
		if ($child->hasUnsaved()) {
		    $this->temp = true;
		    return true;
		}
	    }
	}
	return false;
    }
    public function countUnsavedChildren() {
	$num = 0;
	foreach ($this->child_hashes as $hash) {
	    $child = WordTrailsGlobal::getNode($hash, false);
	    if (is_object($child)) {
		if ($child->hasUnsaved())
		    $num++;
	    }
	}
	return $num;
    }

    public function delete($confirm = false) {
	if ($confirm) {
	    foreach ($this->child_hashes as $hash) {
		$child = WordTrailsGlobal::getNode($hash);
		if (!is_object($child)) continue;
		$child->delete(true);
	    }
	    WordTrailsUtilities::deleteSlugs($this->Hash);
	}
	parent::delete($confirm);
    }

    public function forceDBUpdate() {
	if (!$this->hasUnsaved()) return;
	$this->temp = false;
	$this->update();
	foreach ($this->child_hashes as $hash) {
	    $child = WordTrailsGlobal::getNode($hash, false);
	    if (!is_object($child)) continue;
	    $child->temp = false;
	    $child->update();
	}
    }
    
    protected function getBaseNodeDBInfo() {
	if (parent::getBaseNodeDBInfo()) {
	    if (is_null($this->Slug) || empty($this->Slug)) {
		return $this->saveSlugOnly();
	    }
	    return true;
	}
	return false;
    }

    // {{{ saveChildren()
    public function saveChildren() {
	foreach ($this->child_hashes as $hash) {
	    $child = WordTrailsGlobal::getNode($hash, false);
	    if (!is_object($child)) continue;
    	    $child->save(true);
	}
	parent::saveChildren();
	foreach ($this->child_hashes as $hash) {
	    $child = WordTrailsGlobal::getNode($hash, false);
	    if (!is_object($child)) continue;
    	    $child->save(false);
	}
    }
    // }}}
    
    protected function saveSlugOnly() {
	$this->getUniqueSlug();
    }
    
    public function save($delay_children = false) {
	$this->getUniqueSlug(false);
	parent::save($delay_children);
    }
    
    public function revertToSaved() {
	foreach ($this->child_hashes as $hash) {
	    $node = WordTrailsGlobal::getNode($hash);
	    if (!is_object($node)) continue;
	    $node->revertToSaved();
	}
	parent::revertToSaved();
    }

    // {{{ removeChild($hash)
    public function removeChild($hash) {
	if (!is_array($hash)) $hash = array($hash);
	parent::removeChild($hash);
	foreach ($hash as $h) {
	    $node = WordTrailsGlobal::getNode($h);
	    if (!is_object($node)) continue;
	    $node->delete();
	}
    }
    // }}}

    // {{{ writeChildrenToXML(DOMDocument &$dom)
    protected function writeChildrenToXML(DOMDocument &$dom) {
	$children = $dom->createElement("children");
	foreach ($this->child_hashes as $hash) {
	    $child = WordTrailsGlobal::getNode($hash);
	    if (!is_object($child)) continue;
	    $node = $child->writeToXML($dom);
	    if (false !== array_search($hash,$this->defaults))
		$node->setAttribute("default", 1);
	    if (isset($this->rel_comments[$hash]))
		$node->setAttribute("comment", $this->rel_comments[$hash]);
	    if (isset($this->rel_controls[$hash]))
		$node->setAttribute("control", $this->rel_controls[$hash]);
	    $children->appendChild($node);
	}
	return $children;
    }
    // }}}

    // {{{ display()
    public function displayHREF() {
	$this->getUniqueSlug();
	return WordTrailsUtilities::permalink(null, ($this->temp ? $this->Hash : $this->NodeID));
    }
    // }}}
    
    public function display() {
	$short = $this->getShortDesc();
	$sid = session_id();
	$session =  WordTrailsAnalytics::pullDetailsFor($this->Hash, $sid);
	$follows = null;
	if (isset($session[$this->Hash]) && isset($session[$this->Hash][$sid]))
	    $follows = $session[$this->Hash][$sid]["hit"];
	unset($session);
	if (!empty($follows)) {
		$follows = array_filter($follows, array("WordTrailsUtilities", "onlyWEV"));
		if (!empty($follows)) {
			global $wp_wt_sort;
			$old_sort = $wp_wt_sort;
			$wp_wt_sort = "Stamp";
			uasort($follows, array("WordTrailsUtilities", "array_object_sort"));
			$wp_wt_sort = $old_sort;
			$lastFollowed = end($follows);
			reset($follows);
			$follows = array_map(array("WordTrailsUtilities", "extractNodeHash"), array_values($follows));
		}
	}
	$new_nodes = array();
	foreach ($this->child_hashes as $hash) {
	    $node = WordTrailsGlobal::getNode($hash);
	    if (!is_object($node)) continue;
	    $time = strtotime($node->getCreated());
	    if (abs(time() - $time) < 86400) array_push($new_nodes, $node->getHash());
	}
	//if (!empty($short))
		//echo $short . "<br style=\"clear:both\" />";
	$content = "";
	$commit = "<div id=\"commit\" class=\"hide-if-js\">";
	if ($this->hasUnsaved()) $commit = "<div id=\"commit\">";
	if (current_user_can(WordTrailsGlobal::CAP_SAVE)) {
	    $commit .='<a href="' . WordTrailsUtilities::redirect_link_to("save", $this->Hash) . '" class="ltoh"><img src="' . WordTrailsGlobal::$urlpath . 'imgs/buttons/save_publish_l.png" alt="Save & Publish" /></a>';
	    if (is_int($this->NodeID)) $commit .= ' <a href="' . WordTrailsUtilities::redirect_link_to("revert", $this->Hash) . '" class="ltoh"><img src="' . WordTrailsGlobal::$urlpath . 'imgs/buttons/revert_l.png" alt="Revert to Saved" /></a>';
	} else {
	    $commit .='You do not have permission to save these changes';
	}
	$commit .= "</div>";
	if (current_user_can(WordTrailsGlobal::CAP_SAVE) || $this->hasUnsaved()) $content .= $commit;
	$content .= '<div class="rounded nohover">
		<div class="header">
			<div class="left">
				<div class="float_icon">
					<img src="' . WordTrailsGlobal::$urlpath . 'imgs/curr_trail.png" alt="Trail Icon" />
				</div>
				<div class="full_bottom">
					<div class="trail_title">' . htmlspecialchars($this->info->Name) . '</div>';
	if (!empty($short)) {
	    $content .='
					<div class="trail_caption">' . htmlspecialchars($short) . '</div>';
	}
	$content .= '
				</div>
			</div>
			<div class="right">';
	if (!empty($follows) && 0) {
		$lastFollowedNode = WordTrailsGlobal::getNode($lastFollowed->NodeHash);
		if (is_object($lastFollowedNode)) {
			$content .= '
			    <a href="' . $lastFollowedNode->displayHREF() . '" class="ltoh" title="Return to ' . $lastFollowedNode->post_info->Title . '"><img src="' . WordTrailsGlobal::$urlpath . 'imgs/buttons/wev_l.png" alt="Return to ' . $lastFollowedNode->post_info->Title . '" /></a>';
		}
	}
	$content .='
				<a href="' . WordTrailsUtilities::appendVar(WordTrailsUtilities::redirect_link_to("print", $this->NodeID),"button", 2, true) . '" rel="external" class="ltoh" title="Print Trail"><img src="' . WordTrailsGlobal::$urlpath . 'imgs/buttons/print_l.png" alt="Print Trail" /></a>
				<a href="' . WordTrailsUtilities::redirect_link_to("xml", $this->Hash) . '" class="ltoh" title="View Trail XML"><img src="' . WordTrailsGlobal::$urlpath . 'imgs/buttons/xml_l.png" alt="View XML" /></a>
			</div>
		</div>
		<div id="BEV">';
	
	$xml = WordTrailsUtilities::redirect_link_to("xml", $this->Hash);
	$abort_display = true;
	$flashvars = array();
	if (!empty($follows)) $flashvars["follows"] = str_replace('"', "'", json_encode($follows));
	if (!empty($new_nodes)) $flashvars["new_nodes"] = str_replace('"', "'", json_encode($new_nodes));
	if (current_user_can(WordTrailsGlobal::CAP_SAVE) || $this->hasUnsaved()) {
		$flashvars["save_uri"] = urlencode(WordTrailsUtilities::redirect_link_to("tv_save", 1));
		$flashvars["redirect_after_save"] = urlencode(WordTrailsUtilities::permalink(null, $this->Hash));
		include "autosave.inc";
	}
	include "visual_trail_long.inc";
	$content .= $trailViewer;
	$content .= '
		</div>
	</div>';
	return $content;
    }
    
    public function displayInList() {
	$followcount = WordTrailsAnalytics::counts($this->Hash);
	$length = $this->length();
	$short = $this->getShortDesc();
	$pop = array();
	$pop = $this->gatherPopupInfo();
	if (count($pop) < 3) $pop = array();
	$flag = "<img src=\"" . WordTrailsGlobal::$urlpath . "imgs/flags/%file%.png\" alt=\"%alt%\" title=\"%title%\" class=\"flag\"/>";
	$find = array("/\%file\%/", "/\%alt\%/", "/\%title\%/");
	if ($this->hasUnsaved()) {
	    $rpl = array("editing", "Editing Flag", "This trail has unsaved changes");
	    $flag = preg_replace($find, $rpl, $flag);
	} elseif (abs(time() - strtotime($this->Created)) < 86400) {
	    $rpl = array("new", "New Flag", "This trail was created " . WordTrailsUtilities::stampToAbbr($this->Created, true));
	    $flag = preg_replace($find, $rpl, $flag);
	    $flag .= "<span class=\"flag\">" . WordTrailsUtilities::stampToAbbr($this->Created) . "</span>";
	} elseif (abs(time() - strtotime($this->Modified)) < 86400) {
	    $rpl = array("updated", "Updated Flag", "This trail was updated " . WordTrailsUtilities::stampToAbbr($this->Modified, true));
	    $flag = preg_replace($find, $rpl, $flag);
	    $flag .= "<span class=\"flag\">" . WordTrailsUtilities::stampToAbbr($this->Modified) . "</span>";
	} else {
	    $flag = "";
	}
	
	$content = "
<div class=\"rounded" . (!empty($pop) ? " tipthis\" title=\"" . str_replace('"', "'", json_encode($pop)) : "") . "\">
	<div class=\"header\">
	    <div class=\"left\">
		<div class=\"float_icon\">
		    <img src=\"" . WordTrailsGlobal::$urlpath . "imgs/curr_trail.png\" alt=\"Trail Icon\" />
		</div>
	    <div class=\"full_bottom\">
		<div class=\"trail_title\"><a href=\"". $this->displayHREF() . "\">" . htmlspecialchars($this->info->Name) . "</a>{$flag}</div>";
	if (!empty($short)) {
	    $content .='
		<div class="trail_caption">' . htmlspecialchars($short) . '</div>';
	}
	$content .='
	    </div>
	</div>
	<div class="right">
	    <a href="' . WordTrailsUtilities::appendVar(WordTrailsUtilities::redirect_link_to("print", $this->NodeID), "button", 3, true) . '" rel="external" class="ltoh" title="Print Trail"><img src="' . WordTrailsGlobal::$urlpath . 'imgs/buttons/print_l.png" alt="Print Trail" title="Print Trail" /></a>
	    <a href="' . $this->displayHREF() . '" class="ltoh" title="View Trail Map"><img src="' . WordTrailsGlobal::$urlpath . 'imgs/buttons/view_map_l.png" alt="View Map" title="View Trail Map" /></a>
	</div>
    </div>
    <div class="footer">
	<div class="length">
	    <strong>Length:</strong> ' . $length . ' markers
	</div>
	<div class="reading">
	    <strong>Browse Time:</strong> ' . WordTrailsUtilities::browseTime($length) . '
	</div>
	<div class="walks"><div class="walkcount">' . $followcount . '</div></div>';
	if (!empty($this->defaults) && 0) {
	    $content .='
	<div class="start">';
	    foreach ($this->defaults as $hash) {
		$node = WordTrailsGlobal::getNode($hash);
		if (!is_object($node)) continue;
		$content .= "<a href=\"" . $node->displayHREF() . "\" title=\"Start Following\">" . $node->getName() . "</a>";
		break;
	    }
	    $content .='
	</div>';
	}
	$content .='
    </div>
</div>';
	return $content;
    }
    
    public function crawl_defaults(&$defaults = array(), &$alternates = null) {
	if (empty($this->defaults)) return;
	foreach ($this->defaults as $hash) {
	    $node = WordTrailsGlobal::getNode($hash);
	    if (!is_object($node)) continue;
	    if ($node->getType() != "node") continue;
	    array_push($defaults, $hash);
	    $node->crawl_defaults($defaults, $alternates);
	    break;
	}
	if (is_array($alternates)) $alternates = array_unique(array_merge($alternates, array_diff($this->child_hashes, $defaults)));
    }
    
    public function length() {
	$nodes = array();
	$this->crawl_defaults($nodes);
	return count($nodes);
    }
    
    /* for future version
    public function getPrintNode() {
	foreach ($this->child_hashes as $hash) {
	    $node = WordTrailsGlobal::getNode($hash, false);
	    if (!is_object($node)) continue;
	    if ($node->getType() == "Print") return $node;
	}
	return null;
    }
    //*/

    public function gatherPrintNodes() {
	//$this->sort_children("created", true);
	$nodes = array();
	$alternates = array();
	/*foreach ($this->defaults as $hash) {
	    array_push($nodes, $hash);
	    $node = WordTrailsGlobal::getNode($hash);
	    if (!is_object($node)) continue;
	    $node->crawl_defaults($nodes,$alternates);
	}
	*/
	$this->crawl_defaults($nodes, $alternates);
	$alternates = array_diff($alternates, $nodes);
	echo "First Crawl";
	superprint(array("Nodes"=>$nodes, "Alternates"=>$alternates), true);
	$sections = 1;
	while(!empty($alternates)) {
	    $hash = array_shift($alternates);
	    $sections++;
	    array_push($nodes, $hash);
	    $node = WordTrailsGlobal::getNode($hash);
	    if (!is_object($node)) continue;
	    echo "Node Crawl";
	    $node->crawl_defaults($nodes, $alternates);
	    $nodes = array_intersect(array_unique($nodes), $this->child_hashes);
	    $alternates = array_intersect(array_unique(array_diff($alternates, $nodes)), $this->child_hashes);
	    superprint(array("Nodes"=>$nodes, "Alternates"=>$alternates), true);
	}
	$missing = array_diff($this->child_hashes, $nodes);
	superprint(array("Missing"=>$missing));
	while(!empty($missing)) {
	    $hash = array_shift($missing);
	    $sections++;
	    array_push($nodes, $hash);
	    $node = WordTrailsGlobal::getNode($hash);
	    if (!is_object($node)) continue;
	    echo "Node Crawl";
	    $node->crawl_defaults($nodes, $alternates);
	    $nodes = array_intersect(array_unique($nodes), $this->child_hashes);
	    $alternates = array_intersect(array_unique(array_diff($alternates, $nodes)), $this->child_hashes);
	    superprint(array("Nodes"=>$nodes, "Alternates"=>$alternates), true);
	    while(!empty($alternates)) {
		$hash = array_shift($alternates);
		array_push($nodes, $hash);
		$node = WordTrailsGlobal::getNode($hash);
		if (!is_object($node)) continue;
		echo "Node Crawl";
		$node->crawl_defaults($nodes, $alternates);
		$nodes = array_intersect(array_unique($nodes), $this->child_hashes);
		$alternates = array_intersect(array_unique(array_diff($alternates, $nodes)), $this->child_hashes);
		superprint(array("Nodes"=>$nodes, "Alternates"=>$alternates), true);
	    }
	    $missing = array_diff($this->child_hashes, $nodes);
	}
	return array($nodes, $sections);
    }
    
    public function is_empty() {
	if (!empty($this->child_hashes)) return false;
	if (!empty($this->info->Name)) return false;
	if (!empty($this->info->ShortDesc)) return false;
	if (!empty($this->info->LongDesc)) return false;
	return true;
    }

    public function __sleep() {
	if (parent::hasUnsaved()) $this->temp = true;
	if (!$this->temp) {
	    return array();
	} else {
	    if ($this->is_empty()) {
		if (time() - strtotime($this->Created) > 60) {
		    return array();
		}
	    }
	}
	return array_keys(get_object_vars($this));
    }

}
// }}}
?>