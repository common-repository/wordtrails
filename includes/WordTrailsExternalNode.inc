<?php
/**
 * This is the External Node class - it extends the basic node class
 *
 * This file contains the WordTrailsExternalNode class, the node that represents
 * any node external of this blog.
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
 * External Node class
 *
 * This is the External node class. It is used to represent an external node
 * It extends the basic Node class.
 *
 * @package WordTrails
 * @since 0.2.0
 * @author Jesse Silverstein <jesse.silverstein@xerox.com>
 * @copyright 2008 XIG: SemPrint
 * @global WordPress $wpdb WordPress database management class
 */
class WordTrailsExternalNode extends WordTrailsNode {
    // {{{ Properties
    protected $URL = null;
    protected $Site = null;
    protected $XML = null;
    protected $isInternal = false;
    // }}}

    // {{{ initFromXML($XMLNode)
    public function initFromXML(DOMElement &$XMLNode) {
	$this->XML = "" . $XMLNode;
	parent::initFromXML($XMLNode);
    }
    // }}}

    public function setSite($site) {
	if ($site != $this->site) {
	    $this->site = $site;
	    $this->log("Changed site: $site");
	}
    }

    public function setURL($URL) {
	if ($URL != $this->URL) {
	    $this->URL = $URL;
	    $this->log("Changed URL: $URL");
	}
    }

    public function setXML($XML) {
	if ($XML != $this->XML) {
	    $this->XML = $XML;
	    $this->log("Changed XML: $XML");
	}
    }

    public function deriveSite() {
	if (is_null($this->URL)) return false;
	$spl = explode("//", $this->URL);
	if (strstr($spl[1], "/")) {
	    $spl[1] = implode("/",array_pop(explode($spl[1], "/")));
	}
	$this->setSite(implode("//", $spl));
    }

    public function deriveType() {
	if (is_null($this->XML) || empty($this->XML)) {
	    $this->Type = "node";
	} else {
	    $this->Type = "trail";
	}
    }

    // {{{ XMLParse ($XMLNode)
    public function XMLParse (DOMElement &$XMLNode) {
	switch (strtolower($XMLNode->nodeName)) {
	    case "source_site":
		if (!$XMLNode->hasChildNodes()) break;
		$this->setSite($XMLNode->firstChild->nodeValue);
		break;
	    case "source_url":
		if (!$XMLNode->hasChildNodes()) break;
		$this->setURL($XMLNode->firstChild->nodeValue);
		break;
	}
    }
    // }}}

    public function save($delay_children = false) {
	global $wpdb;
	$vals = array( "URL", "Site", "XML");
	$sql = "INSERT INTO " . WordTrailsGlobal::$tables->external . " (ExternalID,";
	$sql .= implode(", ", $vals);
	$sql .= ") VALUES (";
	if (is_null($this->ReferenceID)) $sql .= "NULL"; else $sql .= "$this->ReferenceID";
	foreach ($vals as $val) {
	    if (isset($this->$val) && !is_null($this->$val) && !empty($this->$val)) {
		$sql .= $wpdb->prepare(", %s", $this->$val);
	    } else {
		$sql .= ", NULL";
	    }
	}
	$sql .= ") ON DUPLICATE KEY UPDATE ExternalID = LAST_INSERT_ID(ExternalID), URL = VALUES(URL), Site = VALUES(Site), XML = VALUES(XML)";
	$wpdb->query($sql);
	$this->ReferenceID = (int)$wpdb->insert_id;
	if ($this->ReferenceID == 0) {
	    $findIDsql = "SELECT ExternalID FROM " . WordTrailsGlobal::$tables->external . " WHERE URL LIKE %s AND Site LIKE %s and XML LIKE %s";
	    $this->ReferenceID = (int)$wpdb->get_var($wpdb->prepare($findIDsql, $this->URL, $this->Site, $this->XML));
	}
	parent::save();
    }

    public function update() {
	if ($this->temp) return false;
	$results = parent::update();
	$results["external"] = $this->getExternalDBInfo();
	return $results;
    }

    protected function getExternalDBInfo() {
	if ($this->temp || !is_numeric($this->ReferenceID)) return false;
	global $wpdb;
	$sql = "SELECT URL, Site, XML FROM " . WordTrailsGlobal::$tables->external . " WHERE ExternalID = $this->ReferenceID";
	$info = $wpdb->get_row($sql, OBJECT);
	if (!is_object($info)) return false;
	$this->setURL($info->URL);
	$this->setSite($info->Site);
	$this->setXML($info->XML);
	return true;
    }

    // {{{ displayHREF()
    public function displayHREF() {
	return WordTrailsUtilities::redirect_link_to("ext", $this->NodeID);
    }
    // }}}

    // {{{ display()
    public function display() {
	//stub
	wp_redirect($this->URL);
	exit;
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