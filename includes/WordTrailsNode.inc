<?php
/**
 * This is the basic Node class
 *
 * This file contains the WordTrailsNode class, the basis for all elements of a trail.
 *
 * @package WordTrails
 * @since 0.2.0
 * @author Jesse Silverstein <jesse.silverstein@xerox.com>
 * @copyright 2008 XIG: SemPrint
 * @global WordPress $wpdb WordPress database management class
 */

// {{{ WordTrailsNode
/**
 * Basic Node class
 *
 * This is the basic node, used for building trails. Trails are an extension of
 * this class. Any specific kind of node will be an extension of this class
 * I.E. trails, externals, word docs, pdfs...
 *
 * @package WordTrails
 * @since 0.2.0
 * @author Jesse Silverstein <jesse.silverstein@xerox.com>
 * @copyright 2008 XIG: SemPrint
 * @global WordPress $wpdb WordPress database management class
 */

class WordTrailsNode {
    // {{{ Properties/Variables
    protected $NodeID;
    protected $Hash;
    protected $Type = null;
    protected $Slug = null;
    protected $isInternal = true;
    protected $ReferenceID = null;

    public $temp = true;
    public $delete_on_save = false;
    protected $stored_before_delete = null;

    protected $info = array(
	"Name" => null,
	"ShortDesc" => null,
	"LongDesc" => null,
    );
    protected $Created = null;
    protected $Modified = null;

    protected $trail_id = null;
    protected $child_ids = array();
    protected $parent_ids = array();

    protected $X = null;
    protected $Y = null;

    public $changelog = array();

    public $trail_hash = null;
    public $child_hashes = array();
    public $parent_hashes = array();
    public $defaults = array();

    public $tag_ids = array();
    public $rel_comments = array();
    public $rel_controls = array();
    // }}}

    // {{{ Constructor
    public function __construct($hash, $id=null) {
	$this->info = (object)$this->info;
	$this->Hash = $hash;
	if (!is_null($id)) {
	    $this->overrideNodeID($id);
	    $this->temp = false;
	    $this->update();
	}
	if ($this->temp) {
	    $this->NodeID = WordTrailsGlobal::makeTempNodeID($hash);
	    $this->Created = date("Y-m-d H:i:s");
	    $this->log("Set Created: $this->Created");
	}
    }
    //}}}

    // {{{ __sleep()
    public function __sleep() {
	if ($this->hasUnsaved()) $this->temp = true;
	if (!$this->temp) {
	    return null;
	}
	return array_keys(get_object_vars($this));
    }
    // }}}

    // {{{ __destruct()
    public function __destruct() {
	$this->__sleep();
    }
    // }}}

    // {{{ dynamic accessor
    public function __call($method, $args) {
	$prefix = strtolower(substr($method, 0, 3));
        $property = substr($method, 3);
        if (empty($prefix) || empty($property) || $prefix != "get")
            return;
        if (isset($this->$property))
            return $this->$property;
	if (isset($this->info->$property))
	    return $this->info->$property;
	return null;
    }
    // }}}

    // {{{ Methods

    protected function overrideNodeID($id) {
	if (!is_numeric($id)) return false;
	$this->NodeID = $id;
	WordTrailsGlobal::makeInstance();
	WordTrailsGlobal::$instance->nodeHashToID[$this->Hash] = $id;
	WordTrailsGlobal::$instance->nodeIDToHash[$id] = $this->Hash;
    }

    // {{{ touch()
    public function touch() {
	$this->Modified = date("Y-m-d H:i:s");
	$this->temp = true;
    }
    // }}}

    public function setName($name) {
	if ($name != $this->info->Name) {
	    $this->info->Name = sanitize_text_field($name);
	    $this->log("Changed name: $name");
	    if (!is_null($this->Slug)) $this->getUniqueSlug(true);
	}
    }
    
    protected function getUniqueSlug($force = false) {
	if (!$force && !is_null($this->Slug)) return;
	$orig_slug = sanitize_title($this->info->Name);
	$unique_slug = $orig_slug;
	if ($unique_slug == "") {
	    $this->Slug = null;
	    return;
	}
	$i = 1;
	while(!WordTrailsUtilities::isUniqueSlug($unique_slug)) {
	    $unique_slug = $orig_slug . "-" . $i;
	    $i++;
	}
	if ($force && !is_null($this->Slug) && !empty($this->Slug) && $unique_slug != $this->Slug) WordTrailsUtilities::recordOldSlug($this->Slug, $this->Hash);
	$this->Slug = $unique_slug;
	if (is_int($this->NodeID)) {
	    $sql = "UPDATE " . WordTrailsGlobal::$tables->node . " SET Slug = %s WHERE Hash = %s";
	    global $wpdb;
	    $wpdb->query($wpdb->prepare($sql, $this->Slug, $this->Hash));
	}
    }

    public function setShort($short) {
	if ($short != $this->info->ShortDesc) {
	    $this->info->ShortDesc = sanitize_text_field($short);
	    $this->log("Changed Short Description: $short");
	}
    }

    public function setLong($Long) {
	if ($Long != $this->info->LongDesc) {
	    $this->info->LongDesc = sanitize_text_field($Long);
	    $this->log("Changed Long Description: $Long");
	}
    }

    public function setX($X) {
	if ((float)$X != $this->X) {
	    $this->X = (float)$X;
	    $last_unsaved = array_pop($this->unsaved());
	    if (false !== stripos($last_unsaved, "Changed Y:")) {
		array_pop($this->changelog);
		$this->log("Changed X,Y: $this->X,$this->Y");
	    } else {
		$this->log("Changed X: $X");
	    }
	}
    }

    public function setY($Y) {
	if ((float)$Y != $this->Y) {
	    $this->Y = (float)$Y;
	    $last_unsaved = array_pop($this->unsaved());
	    if (false !== stripos($last_unsaved, "Changed X:")) {
		array_pop($this->changelog);
		$this->log("Changed X,Y: $this->X,$this->Y");
	    } else {
		$this->log("Changed Y: $Y");
	    }
	}
    }

    public function setCreated($Created) {
	if ($Created != $this->Created) {
	    $this->Created = $Created;
	    $this->log("Changed Created: $Created");
	}
    }

    public function setModified($Modified = null) {
	if (is_null($Modified)) $Modified = date("Y-m-d H:i:s");
	if ($Modified != $this->Modified) {
	    $this->Modified = $Modified;
	    $this->log("Changed Modified: $Modified");
	}
    }

    public function editInfoForm() {
	$disabled = $this->delete_on_save ? " disabled=\"disabled\"" : ""; ?>

<label>Name:<br />
<input type="text" name="process[info][args][name]" value="<?php echo $this->getName(); ?>" <?php echo wt_text_box_size(15); ?><?php echo $disabled; ?> />
</label><br /><br />
<label>Caption:<br />
<textarea name="process[info][args][short]" <?php echo wt_text_area_size(2, 14); ?><?php echo $disabled; ?>><?php echo $this->getShortDesc(); ?></textarea>
</label><br /><br />
<label>Description:<br />
<textarea name="process[info][args][long]" <?php echo wt_text_area_size(5, 14); ?><?php echo $disabled; ?>><?php echo $this->getLongDesc(); ?></textarea>
</label><br /><br />
<?php
    }

    public function editInfoFormProcess($args) {
	extract($args);
	if (isset($name)) $this->setName($name);
	if (isset($short)) $this->setShort($short);
	if (isset($long)) $this->setLong($long);
    }

    public function log($str) {
	$this->changelog[] = $str;
	$this->touch();
    }

    public function unsaved() {
	if (false === array_search("Saved to Database", array_values($this->changelog)))
	    return $this->changelog;
	$lastsave = array_pop(array_keys(array_values($this->changelog), "Saved to Database"));
	return array_slice($this->changelog, $lastsave+1);
    }

    public function hasUnsaved() {
	if (count($this->changelog) == 0) return false;
	if (false === array_search("Saved to Database", array_values($this->changelog))) return true;
	return (count($this->unsaved()) > 0);
    }

    // {{{ hasChildren()
    public function hasChildren() {
	if (count($this->child_hashes) > 0) return true;
	return false;
    }
    // }}}

    public function getLiveChildrenAndHashes() {
	$live_children = array();
	$hashes_from_db = array();
	foreach ($this->child_hashes as $hash) {
	    if (is_object(WordTrailsGlobal::getNode($hash, false))) {
		$live_children[] = $hash;
	    } else {
		$hashes_from_db[] = $hash;
	    }
	}
	return array($live_children, $hashes_from_db);
    }

    public function getLastChild($exclude) {
	if (!is_array($exclude)) $exclude=array($exclude);
	$hashes_with_dates = WordTrailsGlobal::getCreationDate(array_diff($this->child_hashes, $exclude));
	arsort($hashes_with_dates);
	return array_shift(array_keys($hashes_with_dates));
    }

    public function gatherPopupInfo() {
	$pop = array();
	foreach ($this->info as $key => $val) {
	    if (!is_null($val)) {
		$pop[$key] = addslashes(htmlentities($val, ENT_QUOTES));
	    }
	}
	WordTrailsGlobal::makeInstance();
	$tags = WordTrailsGlobal::$instance->tag_manager->tag($this->tag_ids);
	$tags = array_map(create_function('$a', 'return addslashes(htmlentities($a, ENT_QUOTES));'), $tags);
	if (!empty($tags)) $pop["tags"] = $tags;
	return $pop;
    }

    public function delete($confirm = false) {
	$this->flagForDeletion();
	if ($confirm) {
	    $this->deleteFromDB();
	}
    }
    protected function flagForDeletion() {
	if ($this->delete_on_save) return true;
	$this->delete_on_save = true;
	$this->stored_before_delete = serialize($this->info);
	$this->info->Name = "<i>Deleted</i> - " . $this->info->Name;
	$this->info->ShortDesc = "<i>Will be deleted when changes are saved.</i> - " . $this->info->ShortDesc;
	$this->log("Flagged For Deletion");
    }
    public function unFlagForDeletion() {
	if (!$this->delete_on_save) return false;
	$this->delete_on_save = false;
	if ($this->stored_before_delete)
	    $this->info = unserialize($this->stored_before_delete);
	$this->stored_before_delete = null;
	$this->log("Restored From Deletion Flag");

    }
    protected function deleteFromDB() {
	WordTrailsGlobal::unregisterNode($this->Hash);
	if (!is_numeric($this->NodeID)) return false;
	global $wpdb;
	$sql = "DELETE FROM " . WordTrailsGlobal::$tables->node . " WHERE NodeID = $this->NodeID";
	$wpdb->query($sql);
	$sql = "DELETE FROM " . WordTrailsGlobal::$tables->node_rel . " WHERE ParentID = $this->NodeID OR ChildID = $this->NodeID";
	$wpdb->query($sql);
	$sql = "DELETE FROM " . WordTrailsGlobal::$tables->node_tag . " WHERE NodeID = $this->NodeID";
	$wpdb->query($sql);
    }
    
    public function revertToSaved() {
	$this->temp = false;
	$this->changelog = array();
	$this->update();
    }

    // {{{ Database accessors
    // {{{ update()
    public function update() {
	if ($this->temp) return false;
	$results["base"] = $this->getBaseNodeDBInfo();
	$results["trail"] = $this->findTrailID();
	$results["chidlren"] = $this->getChildIDs();
	$results["parents"] = $this->getParentIDs();
	$results["tags"] = $this->getTagIDs();
	return $results;
    }
    // }}}

    // {{{ getBaseNodeDBInfo()
    protected function getBaseNodeDBInfo() {
	if ($this->temp) return false;
	global $wpdb;
	$sql = "SELECT * FROM " . WordTrailsGlobal::$tables->node . " WHERE Hash = %s";
	$result = $wpdb->get_row($wpdb->prepare($sql, $this->Hash));
	if (!is_object($result)) return false;
	//superdump($result,true);
	$this->NodeID = (int)$result->NodeID;
	$this->Slug = $result->Slug;
	$this->ReferenceID = (int)$result->ReferenceID;
	$this->isInternal = (bool)$result->isInternal;
	$this->Type = (string)$result->Type;
	$this->info->Name = sanitize_text_field($result->Name);
	$this->info->ShortDesc = sanitize_text_field($result->ShortDesc);
	$this->info->LongDesc = sanitize_text_field($result->LongDesc);
	$this->Created = $result->Created;
	$this->Modified = $result->Modified;
	$this->X = is_numeric($result->X) ? (float)$result->X : null;
	$this->Y = is_numeric($result->Y) ? (float)$result->Y : null;
	return true;
    }
    // }}}

    // {{{ getChildIDs()
    protected function getChildIDs() {
	if ($this->temp || !is_numeric($this->NodeID)) return false;
	global $wpdb;
	$sql = "SELECT ChildID, `Def`, `Comment` FROM " . WordTrailsGlobal::$tables->node_rel . " WHERE ParentID = %d";
	$children = $wpdb->get_results($wpdb->prepare($sql, $this->NodeID), OBJECT);
	if (!is_array($children)) return false;
	foreach ($children as $child) {
	    $this->child_ids[] = $child->ChildID;
	    $hash = WordTrailsGlobal::getNodeHashFromID($child->ChildID);
	    $this->child_hashes[] = $hash;
	    if ($child->Def)
		$this->defaults[] = $hash;
	    if (!is_null($child->Comment) && $child->Comment != "NULL")
		$this->rel_comments[$hash] = sanitize_text_field($child->Comment);
	}
	$sql = "SELECT nr.ChildID as ChildID, rc.ControlID as ControlID FROM " . WordTrailsGlobal::$tables->node_rel . " as nr LEFT JOIN " . WordTrailsGlobal::$tables->rel_control . " as rc ON nr.NodeRelationshipID = rc.RelationshipID WHERE nr.ParentID = %d";
	$controls = $wpdb->get_results($wpdb->prepare($sql, $this->NodeID), OBJECT);
	if (is_array($controls)) {
	    foreach ($controls as $control) {
		$hash = WordTrailsGlobal::getNodeHashFromID($control->ChildID);
		$split = array("auto", "auto");
		if (isset($this->rel_controls[$hash])) {
		    if (false !== strpos($this->rel_controls[$hash], "|"))
			$split = explode("|", $this->rel_controls[$hash]);
		}
		if (is_object(WordTrailsGlobal::$controls->{$control->ControlID})) {
		    $split[(int)WordTrailsGlobal::$controls->{$control->ControlID}->isForward] = WordTrailsGlobal::$controls->{$control->ControlID}->Term;
		}
		$this->rel_controls[$hash] = join("|", $split);
	    }
	}
	return true;
    }
    // }}}

    // {{{ getParentIDs()
    public function getParentIDs($override = false) {
	if (($this->temp && !$override) || !is_numeric($this->NodeID)) return false;
	global $wpdb;
	$sql = "SELECT nr.ParentID FROM " . WordTrailsGlobal::$tables->node_rel . " as nr LEFT JOIN " . WordTrailsGlobal::$tables->node . " as n ON nr.ParentID = n.NodeID WHERE nr.ChildID = %d AND n.Type != 'trail'";
	$parents = $wpdb->get_col($wpdb->prepare($sql, $this->NodeID));
	if (!is_array($parents)) return false;
	$this->parent_ids = $parents;
	$this->parent_hashes = array_values(WordTrailsGlobal::getNodeHashFromID($this->parent_ids));
	return true;
    }
    // }}}

    // {{{ findTrailID()
    protected function findTrailID() {
	if ($this->temp || !is_numeric($this->NodeID)) return false;
	global $wpdb;
	$sql = "SELECT nr.ParentID FROM " . WordTrailsGlobal::$tables->node_rel . " as nr LEFT JOIN " . WordTrailsGlobal::$tables->node . " as n ON nr.ParentID = n.NodeID WHERE nr.ChildID = %d AND n.Type = 'trail'";
	$trails = $wpdb->get_col($wpdb->prepare($sql, $this->NodeID));
	if (!is_array($trails)) return false;
	$this->trail_id = array_shift($trails);
	$this->trail_hash = WordTrailsGlobal::getNodeHashFromID($this->trail_id);
	return true;
    }
    // }}}

    // {{{ getTagIDs()
    protected function getTagIDs() {
	if ($this->temp || !is_numeric($this->NodeID)) return false;
	global $wpdb;
	$sql = "SELECT t.TagID FROM " . WordTrailsGlobal::$tables->tag . " as t LEFT JOIN " . WordTrailsGlobal::$tables->node_tag . " as nt ON t.TagID = nt.TagID WHERE nt.NodeID = %d";
	$tags = $wpdb->get_col($wpdb->prepare($sql, $this->NodeID));
	if (!is_array($tags)) return false;
	$this->tag_ids = array_unique(array_merge($tags, $this->tag_ids));
    }
    // }}}

    // {{{ save()
    public function save($delay_children = false) {
	if ($this->delete_on_save) {
	    $this->deleteFromDB();
	    return false;
	}
	global $wpdb;
	$vars = array("Hash", "Type", "Slug", "isInternal", "ReferenceID", "Name", "ShortDesc", "LongDesc", "X", "Y", "Created", "Modified");
	$sql  = "INSERT INTO " . WordTrailsGlobal::$tables->node . " (NodeID, ";
	$sql .= implode(", ", $vars);
	$sql .= ") VALUES (";
	if (!is_null($this->NodeID) && !empty($this->NodeID) && is_int($this->NodeID)) {
	    $sql .= $wpdb->prepare(" %d ", $this->NodeID);
	} else {
	    $sql .= " NULL ";
	}
	$d = ", %d ";
	$s = ", %s ";
	foreach ($vars as $var) {
	    if (isset($this->$var)) {
		if (!is_null($this->$var) && (!empty($this->$var) || is_bool($this->$var))) {
		    $use = $s;
		    if (is_numeric($this->$var) || is_bool($this->$var))
			$use = $d;
		    $sql .= $wpdb->prepare($use, $this->$var);
		} else {
		    if ($var == "Created" || $var == "Modified") {
			$sql .= ", NOW() ";
		    } else {
			$sql .= ", NULL ";
		    }
		}
	    } else if (isset($this->info->$var)) {
		if (!is_null($this->info->$var) && !empty($this->info->$var)) {
		    $use = $s;
		    if (is_numeric($this->info->$var))
			$use = $d;
		    $sql .= $wpdb->prepare($use, $this->info->$var);
		} else {
		    $sql .= ", NULL ";
		}
	    } else {
		$sql .= ", NULL ";
	    }
	}
	$sql .= ") ON DUPLICATE KEY UPDATE NodeID = LAST_INSERT_ID(NodeID)";
	foreach ($vars as $var) {
	    $sql .= ", $var = VALUES($var)";
	}
	//echo $sql;
	//trigger_error($sql, 512);
	$result = $wpdb->query($sql);
	$newID = (int)$wpdb->insert_id;
	if ($newID == 0) {
	    $findIDsql = "SELECT NodeID FROM " . WordTrailsGlobal::$tables->node . " WHERE Hash = %s";
	    $newID = (int)$wpdb->get_var($wpdb->prepare($findIDsql, $this->Hash));
	}
	$this->overrideNodeID($newID);
	if (!$delay_children) {
	    $this->saveChildren();
	    $this->saveTags();
	    $this->log("Saved to Database");
	    $this->temp = false;
	}
	return $result;
    }
    // }}}

    // {{{ saveChildren()
    public function saveChildren() {
	if (!is_int($this->NodeID)) return false;
	global $wpdb;
	$this->pruneDefaults();
	if (!empty($this->child_hashes)) {
	    $sql = "INSERT INTO " . WordTrailsGlobal::$tables->node_rel . " (NodeRelationshipID, ParentID, ChildID, `Comment`, `Def`) VALUES(NULL, $this->NodeID, %d, %s, %d) ON DUPLICATE KEY UPDATE `Def` = VALUES(`Def`), `Comment` = VALUES(`Comment`), NodeRelationshipID = LAST_INSERT_ID(NodeRelationshipID)";
	    //$current_rel_controls_sql = "SELECT RelControlID, ControlID FROM " . WordTrailsGlobal::$tables->rel_control . " as rc LEFT JOIN " . WordTrailsGlobal::$tables->node_rel . " as nr ON rc.RelationshipsID = nr.NodeRelationshipID WHERE nr.ParentID = %d AND nr.ChildID = (SELECT NodeID FROM " . WordTrailsGlobal::$tables->node . " WHERE Hash = %s)";
	    $current_rel_controls = "SELECT RelControlID, ControlID FROM " . WordTrailsGlobal::$tables->rel_control . " as rc WHERE RelationshipID = %d ORDER BY RelControlID ASC";
	    $update_rel_control = "UPDATE " . WordTrailsGlobal::$tables->rel_control . " SET ControlID = %d WHERE RelControlID = %d";
	    $insert_rel_control = "INSERT INTO " . WordTrailsGlobal::$tables->rel_control . " (RelControlID, RelationshipID, ControlID) VALUES ";
	    $del_rel_control = "DELETE FROM " . WordTrailsGlobal::$tables->rel_control . " WHERE RelationshipID = %d";
	    $this->child_ids = array_filter(WordTrailsGlobal::getNodeIDFromHash($this->child_hashes), is_numeric);
	    foreach ($this->child_hashes as $hash) {
		$cid = WordTrailsGlobal::getNodeIDFromHash($hash);
		if (!is_numeric($cid)) continue;
		$comm = null;
		if (isset($this->rel_comments[$hash])) $comm = $this->rel_comments[$hash];
		$results[$hash] = $wpdb->query($wpdb->prepare($sql, $cid, $comm, ($this->isDefault($hash) ? 1 : 0)));
		$relID = (int)$wpdb->insert_id;
		if ($relID == 0) {
		    $findIDsql = "SELECT NodeRelationshipID FROM " . WordTrailsGlobal::$tables->node_rel . " WHERE ParentID = %d AND ChildID = %d";
		    $relID = (int)$wpdb->get_var($wpdb->prepare($findIDsql, $this->NodeID, $cid));
		}
		if (isset($this->rel_controls[$hash])) {
		    if ($this->rel_controls[$hash] == "" || false === strpos($this->rel_controls[$hash], "|"))
			$this->rel_controls[$hash] = "auto|auto";
		    $split = explode("|", $this->rel_controls[$hash]);
		    $back = WordTrailsGlobal::$controls->{$split[0]};
		    if ($back->Term == "auto") $back = WordTrailsGlobal::$controls->fromTerm("auto", true);
		    $front = WordTrailsGlobal::$controls->{$split[1]};
		    $controls = $wpdb->get_results($wpdb->prepare($current_rel_controls, $relID));
		    if (is_array($controls) && count($controls) == 2) {
			if ($back->ControlID != $controls[0]->ControlID) $wpdb->query($wpdb->prepare($update_rel_control, $back->ControlID, $controls[0]->RelControlID));
			if ($front->ControlID != $controls[1]->ControlID) $wpdb->query($wpdb->prepare($update_rel_control, $front->ControlID, $controls[1]->RelControlID));
		    } else {
			if (!empty($controls)) {
			    $wpdb->query($wpdb->prepare($del_rel_control, $relID));
			}
			$wpdb->query($insert_rel_control . "(NULL, {$relID}, {$back->ControlID}), (NULL, {$relID}, {$front->ControlID})");
		    }
		}
		WordTrailsGlobal::removeUnsavedChild($this->Hash, $hash);
	    }
	    if (!empty($this->child_ids)) {
		$sql = "SELECT NodeRelationshipID FROM " . WordTrailsGlobal::$tables->node_rel . " WHERE ParentID = $this->NodeID AND ChildID NOT IN (" . implode(",", $this->child_ids) . ")";
		$del_ids = $wpdb->get_col($sql);
		if (!empty($del_ids)) {
		    $sql = "DELETE FROM " . WordTrailsGlobal::$tables->rel_control . " WHERE RelationshipID IN (" . implode(",", $del_ids) . ")";
		    $wpdb->query($sql);
		    $sql = "DELETE FROM " . WordTrailsGlobal::$tables->node_rel . " WHERE NodeRelationshipID IN (" . implode(",", $del_ids) . ")";
		    $wpdb->query($sql);
		}
	    }
	    $this->log("Saved Children to Database");
	} else {
	    $sql = "DELETE FROM " . WordTrailsGlobal::$tables->rel_control . " WHERE RelationshipID IN(SELECT NodeRelationshipID FROM " . WordTrailsGlobal::$tables->node_rel . " WHERE ParentID = {$this->NodeID})";
	    $wpdb->query($sql);
	    $sql = "DELETE FROM " . WordTrailsGlobal::$tables->node_rel . " WHERE ParentID = $this->NodeID";
	    $wpdb->query($sql);
	}
	return $results;
    }
    // }}}

    // {{{ saveTags()
    public function saveTags() {
	if (!is_int($this->NodeID)) return false;
	global $wpdb;
	//$this->updateTagIDs();
	$sql = "INSERT INTO " . WordTrailsGlobal::$tables->node_tag . " (NodeTagID, NodeID, TagID) VALUES (NULL, $this->NodeID, %d) ON DUPLICATE KEY UPDATE NodeTagID = LAST_INSERT_ID(NodeTagID)";
	foreach ($this->tag_ids as $tid) {
	    if (!is_int($tid)) continue;
	    $results[$tid] = $wpdb->query($wpdb->prepare($sql, $tid));
	}
	if (!empty($this->tag_ids)) {
	    $sql = "DELETE FROM " . WordTrailsGlobal::$tables->node_tag . " WHERE NodeID = {$this->NodeID} AND TagID NOT IN (" . implode(",", $this->tag_ids) . ")";
	    $wpdb->query($sql);
	}
	$this->log("Saved Tags to Database");
	return $results;
    }
    // }}}
    // }}}

    // {{{ updateTagIDs
    public function updateTagIDs() {
	global $wpdb;
	WordTrailsGlobal::makeInstance();
	$tags = WordTrailsGlobal::$instance->tag_manager->tag($this->tag_ids);
	$this->tag_ids = WordTrailsGlobal::$instance->tag_manager->addTag($tags);
	$this->log("Updated tags from TagManager");
    }
    // }}}

    // {{{ getDefaultChildren([$commonParentHash = null])
    public function getDefaultChildren($commonParentHash = null) {
	if (is_null($commonParentHash)) return $this->defaults;
	$commonParentHash = WordTrailsGlobal::getNodeHashFromID($commonParentHash);
	$commonParent = WordTrailsGlobal::getNode($commonParentHash);
	if (!is_object($commonParent)) return false;
	if (false === array_search($this->Hash, $commonParent->child_hashes)) return null;
	return array_intersect($this->child_hashes, $commonParent->child_hashes);
    }
    // }}}

    public function isDefault($hash) {
	return (false !== array_search($hash, $this->defaults));
    }

    protected function pruneDefaults() {
	$existing_defaults = array();
	foreach ($this->defaults as $hash) {
	    if (WordTrailsGlobal::hashExists($hash))
		$existing_defaults[] = $hash;
	}
	$existing_defaults = array_intersect($existing_defaults, $this->child_hashes);
	if (empty($existing_defaults) && !empty($this->child_hashes))
	    $existing_defaults[] = $this->child_hashes[0];
	$this->defaults = $existing_defaults;
    }

    public function addChild($hash, $default = false) {
	if (!is_array($hash)) $hash = array($hash);
	if (count(array_diff($hash, $this->child_hashes))) {
	    $this->child_hashes = array_unique(array_merge($this->child_hashes, $hash));
	    if ($default)
		$this->defaults = array_unique(array_merge($this->defaults, $hash));
	    if (empty($this->defaults))
		array_push($this->defaults, $hash[0]);
	    foreach ($hash as $child) {
		WordTrailsGlobal::storeUnsavedChild($this->Hash, $child);
	    }
	    $this->log("Added Child hashes: " . implode($hash) . ($default ? " as default" : ""));
	}
    }

    public function removeChild($hash) {
	if (!is_array($hash)) $hash = array($hash);
	$this->log("Removing child hashes: " . implode(", ",$hash));
	if (count(array_intersect($hash, $this->child_hashes)) > 0) {
	    $this->child_hashes = array_diff($this->child_hashes, $hash);
	    $this->defaults = array_diff($this->defaults, $hash);
	    foreach ($hash as $child) {
		WordTrailsGlobal::removeUnsavedChild($this->Hash, $child);
	    }
	    $this->log("Removed child hashes: " . implode(", ",$hash));
	}
    }

    public function hasChild($hash) {
	return false !== array_search($hash, $this->child_hashes);
    }

    public function setRelationshipComment($hash, $comment) {
	$orig = $this->rel_comments[$hash];
	$comment = sanitize_text_field($comment);
	if (!isset($this->rel_comments[$hash]) || $this->rel_comments[$hash] != $comment) {
	    $this->rel_comments[$hash] = $comment;
	    if ($orig != $this->rel_comments[$hash]) $this->log("Changed Relationship Comment ($hash): $comment");
	}
    }
    
    public function setRelationshipControl($hash, $control) {
	$orig = $this->rel_controls[$hash];
	if ($control == "" || false === strpos($control, "|") || $control == "auto|auto")
	    $this->rel_controls[$hash] = "auto|auto";
	else {
	    $split = explode("|", $control);
	    $this->rel_controls[$hash] = "" . WordTrailsGlobal::$controls->{$split[0]}->Term . "|" . WordTrailsGlobal::$controls->{$split[1]}->Term;
	}
	if ($orig != $this->rel_controls[$hash]) $this->log("Changed Relationship Controls ($hash): " . $this->rel_controls[$hash]);
    }

    public function getComment($hash) {
	if (is_array($hash)) {
	    $comm = array();
	    foreach ($hash as $h) {
		$comm[$h] = $this->getComment($h);
	    }
	    return $comm;
	}
	if (isset($this->rel_comments[$hash])) return $this->rel_comments[$hash];
	else return null;
    }

    public function addDefaults($hash, $override_log = false) {
	if (!is_array($hash)) $hash = array($hash);
	if (count(array_diff($hash, $this->defaults))) {
	    $this->defaults = array_unique(array_merge($this->defaults, $hash));
	    if (!$override_log) $this->log("Added defaults: " . implode($hash));
	}
    }

    public function removeDefaults($hash) {
	if (!is_array($hash)) $hash = array($hash);
	if (count(array_intersect($hash, $this->defaults))) {
	    $this->defaults = array_diff($this->defaults, $hash);
	    $this->log("Removed defaults: " . implode($hash));
	}
    }

    //DO _NOT_ USE WITHOUT ADDING A NEW ONE
    protected function clearDefaults($override_log = false) {
	$this->defaults = array();
	if (!$override_log) $this->log("Cleared defaults");
    }

    public function setOnlyDefault($hash) {
	$this->clearDefaults(true);
	$this->addDefaults($hash, true);
	$this->log("Set default: $hash");
    }

    public function addTag($tag) {
	if (!is_array($tag)) $tag = array($tag);
	WordTrailsGlobal::makeInstance();
	$tids = WordTrailsGlobal::$instance->tag_manager->addTag($tag);
	$compare = array_diff($tids, $this->tag_ids);
	if (count($compare)) {
	    $this->tag_ids = array_unique(array_merge($this->tag_ids, $tids));
	    $this->log("Added Tag(s): " . implode(", ", WordTrailsGlobal::$instance->tag_manager->tag($compare)));
	}
    }

    public function removeTag($tag) {
	if (!is_array($tag)) $tag = array($tag);
	WordTrailsGlobal::makeInstance();
	$remove_tids = WordTrailsGlobal::$instance->tag_manager->tid($tag);
	$compare = array_intersect($this->tag_ids, $remove_tids);
	if (count($compare)) {
	    $this->tag_ids = array_diff($this->tag_ids, $remove_tids);
	    $this->log("Removed Tag(s): " . implode(", ", WordTrailsGlobal::$instance->tag_manager->tag($compare)));
	}
    }

    public function setTags($tags) {
	if (!is_array($tags)) $tags = array($tags);
	WordTrailsGlobal::makeInstance();
	$tids = WordTrailsGlobal::$instance->tag_manager->addTag($tags);
	$tids = array_filter($tids, is_numeric);
	$tids = array_map(array("WordTrailsNode", "toInt"), $tids);
	$add_tids = array_diff($tids, $this->tag_ids);
	$rem_tids = array_diff($this->tag_ids, $tids);
	if (!empty($add_tids) > 0) {
	    $this->log("Added Tag(s): " . implode(", ", $add_tids));
	}
	if (count($rem_tids) > 0) {
	    $this->log("Removed Tag(s): " . implode(", ", $rem_tids));
	}
	$this->tag_ids = $tids;
    }
    public static function toInt($s) {
	return (int)$s;
    }

    // {{{ XML Functions
    // {{{ initFromXML($XMLNode)
    /**
     * assumes type= and hash= have already been processed by Global
     * before this object was even created
     */
    public function initFromXML(DOMElement &$XMLNode) {
	if (!$XMLNode->hasChildNodes()) return false;
	foreach ($XMLNode->childNodes as $child) {
	    $empty = false;
	    if ((empty($child->nodeValue) || is_null($child->nodeValue)) && !$child->hasChildNodes() && !$child->hasAttributes() && strtolower($child->nodeName) != "children") $empty = true;
	    switch (strtolower($child->nodeName)) {
		case "id":
		    if ($empty || !is_numeric(WordTrailsUtilities::extractXMLNode($child))) break;
		    $this->NodeID = (int)WordTrailsUtilities::extractXMLNode($child);
		    $this->temp = false;
		    break;
		case "reference_id":
		    if ($empty) break;
		    $this->ReferenceID = (int)WordTrailsUtilities::extractXMLNode($child);
		    break;
		case "name":
		    $this->setName(WordTrailsUtilities::extractXMLNode($child));
		    break;
		case "short_desc":
		    $this->setShort(WordTrailsUtilities::extractXMLNode($child));
		    break;
		case "long_desc":
		    $this->setLong(WordTrailsUtilities::extractXMLNode($child));
		    break;
		case "created":
		    if ($empty) break;
		    $this->setCreated(WordTrailsUtilities::extractXMLNode($child));
		    break;
		case "modified":
		    if ($empty) break;
		    $this->setModified(WordTrailsUtilities::extractXMLNode($child));
		    break;
		case "x":
		    $this->setX((float)WordTrailsUtilities::extractXMLNode($child));
		    break;
		case "y":
		    $this->setY((float)WordTrailsUtilities::extractXMLNode($child));
		    break;
		case "authors":
		    //do something with authors
		    break;
		case "tags":
		    $tags = WordTrailsUtilities::extractXMLNode($child);
		    $this->setTags($tags);
		    break;
		case "children":
		    if ($empty) {
			if (count($this->child_hashes)) $this->removeChild($this->child_hashes);
			break;
		    }
		    $hashes = array();
		    foreach ($child->childNodes as $node) {
			if (strtolower($node->nodeName) != "node") continue; //skip parsing text nodes as children (should not occur)
			$type = $node->getAttribute("type");
			$hash = $node->getAttribute("hash");
			$relcomment = $node->getAttribute("comment");
			$relcontrol = $node->getAttribute("control");
			$default = $node->getAttribute("default");
			$default = (bool)(is_numeric($default) && (int)$default == 1);
			if (empty($type) || is_null($type) || empty($hash) || is_null($hash)) continue; //children must have a type and a hash, otherwise ignore
			$hashes[] = $hash;
			if (strtolower($type) != "reference") { //if child is actual node, build it first
			    WordTrailsGlobal::buildNodeFromXML($node);
			}
			$this->addChild($hash, $default);
			if ($default)
			    $this->setOnlyDefault($hash);
			$this->setRelationshipComment($hash, $relcomment);
			$this->setRelationshipControl($hash, $relcontrol);
		    }
		    $remove_children = array_diff($this->child_hashes, $hashes);
		    if (count($remove_children)) $this->removeChild($remove_children);
		    break;
	    }
	    $this->XMLParse($child);
	}
    }
    // }}}
    
    public function updateFromChangesetXML(DOMElement &$XMLNode) {
	if (!$XMLNode->hasChildNodes()) return false;
	$this->touch();
	foreach ($XMLNode->childNodes as $child) {
	    $empty = false;
	    if ((empty($child->nodeValue) || is_null($child->nodeValue)) && !$child->hasChildNodes() && !$child->hasAttributes() && strtolower($child->nodeName) != "children") $empty = true;
	    switch (strtolower($child->nodeName)) {
		case "name":
		    $this->setName(WordTrailsUtilities::extractXMLNode($child));
		    break;
		case "short_desc":
		    $this->setShort(WordTrailsUtilities::extractXMLNode($child));
		    break;
		case "long_desc":
		    $this->setLong(WordTrailsUtilities::extractXMLNode($child));
		    break;
		case "x":
		    $this->setX((float)WordTrailsUtilities::extractXMLNode($child));
		    break;
		case "y":
		    $this->setY((float)WordTrailsUtilities::extractXMLNode($child));
		    break;
		case "tags":
		    $this->setTags(WordTrailsUtilities::extractXMLNode($child));
		    break;
		case "children":
		    if ($empty) break;
		    $hashes = array();
		    foreach ($child->childNodes as $node) {
			if (strtolower($node->nodeName) != "node") continue; //skip parsing text nodes as children (should not occur)
			$type = $node->getAttribute("type");
			$hash = $node->getAttribute("hash");
			$relcomment = $node->getAttribute("comment");
			$relcontrol = $node->getAttribute("control");
			$default = $node->getAttribute("default");
			$default = (bool)(is_numeric($default) && (int)$default == 1);
			if (empty($type) || is_null($type) || empty($hash) || is_null($hash)) continue; //children must have a type and a hash, otherwise ignore
			$hashes[] = $hash;
			if (strtolower($type) != "reference") { //if child is actual node, build it first
			    WordTrailsGlobal::updateNodeFromChangesetXML($node);
			}
			$this->addChild($hash, $default);
			if ($default)
			    $this->setOnlyDefault($hash);
			$this->setRelationshipComment($hash, $relcomment);
			$this->setRelationshipControl($hash, $relcontrol);
		    }
		    break;
		case "delete":
		    if ($empty) break;
		    $hashes = array();
		    foreach ($child->childNodes as $node) {
			if (strtolower($node->nodeName) != "node") continue; //skip parsing text nodes as children (should not occur)
			$hashes[] = $node->getAttribute("hash");
		    }
		    if (count($hashes)) $this->removeChild($hashes);
		    break;
	    }
	    $this->changesetXMLParse($child);
	}
	return true;
    }

    // {{{ writeToXML(&$dom)
    public function writeToXML(DOMDocument &$dom = null) {
	if (is_null($dom)) $dom = new DOMDocument("1.0");
	$node = &$dom->createElement("node");
	$node->setAttribute("type", $this->Type);
	$node->setAttribute("hash", $this->Hash);
	$node->appendChild($this->writeNodeIDToXML($dom));
	$node->appendChild($this->writeReferenceIDToXML($dom));
	$node->appendChild($this->writeNameToXML($dom));
	$node->appendChild($this->writeShortDescToXML($dom));
	$node->appendChild($this->writeLongDescToXML($dom));
	$node->appendChild($this->writeCreatedToXML($dom));
	$node->appendChild($this->writeModifiedToXML($dom));
	$node->appendChild($this->writeXToXML($dom));
	$node->appendChild($this->writeYToXML($dom));
	$node->appendChild($this->writeSourceURLToXML($dom));
	$node->appendChild($this->writeSourceSiteToXML($dom));
	$node->appendChild($this->writeTagsToXML($dom));
	$node = $this->writeOtherInfoToXML($node, $dom);
	$node->appendChild($this->writeChildrenToXML($dom));
	return $node;
    }
    // }}}

    // {{{ writeNodeIDToXML(&$dom)
    protected function writeNodeIDToXML(&$dom) {
	return WordTrailsUtilities::createBasicXMLNode("id", $this->NodeID, $dom);
    }
    // }}}

    // {{{ writeReferenceIDToXML(&$dom)
    protected function writeReferenceIDToXML(&$dom) {
	return WordTrailsUtilities::createBasicXMLNode("reference_id", $this->ReferenceID, $dom);
    }
    // }}}

    // {{{ writeNameToXML(&$dom)
    protected function writeNameToXML(&$dom) {
	return WordTrailsUtilities::createBasicXMLNode("name", WordTrailsUtilities::plain_quotes($this->info->Name), $dom);
    }
    // }}}

    // {{{ writeShortDescToXML(&$dom)
    protected function writeShortDescToXML(&$dom) {
	return WordTrailsUtilities::createBasicXMLNode("short_desc", WordTrailsUtilities::plain_quotes($this->info->ShortDesc), $dom);
    }
    // }}}

    // {{{ writeLongDescToXML(&$dom)
    protected function writeLongDescToXML(&$dom) {
	return WordTrailsUtilities::createBasicXMLNode("long_desc", WordTrailsUtilities::plain_quotes($this->info->LongDesc), $dom);
    }
    // }}}

    // {{{ writeCreatedToXML(&$dom)
    protected function writeCreatedToXML(&$dom) {
	return WordTrailsUtilities::createBasicXMLNode("created", $this->Created, $dom);
    }
    // }}}

    // {{{ writeModifiedToXML(&$dom)
    protected function writeModifiedToXML(&$dom) {
	return WordTrailsUtilities::createBasicXMLNode("modified", $this->Modified, $dom);
    }
    // }}}

    // {{{ writeXToXML(&$dom)
    protected function writeXToXML(&$dom) {
	return WordTrailsUtilities::createBasicXMLNode("x", $this->X, $dom);
    }
    // }}}

    // {{{ writeYToXML(&$dom)
    protected function writeYToXML(&$dom) {
	return WordTrailsUtilities::createBasicXMLNode("y", $this->Y, $dom);
    }
    // }}}

    // {{{ writeSourceURLToXML(DOMDocument &$dom)
    protected function writeSourceURLToXML(DOMDocument &$dom) {
	return WordTrailsUtilities::createBasicXMLNode("source_url", $this->displayHREF(), $dom);
    }
    // }}}

    // {{{ writeSourceSiteToXML(DOMDocument &$dom)
    protected function writeSourceSiteToXML(DOMDocument &$dom) {
	return WordTrailsUtilities::createBasicXMLNode("source_site", get_option('home'), $dom);
    }
    // }}}

    // {{{ writeTagsToXML(&$dom)
    protected function writeTagsToXML(&$dom) {
	WordTrailsGlobal::makeInstance();
	$tags = WordTrailsGlobal::$instance->tag_manager->tag($this->tag_ids);
	return WordTrailsUtilities::explodeToXMLNodes("tags", "tag", $tags, null, $dom);
    }
    // }}}

    // {{{ writeOtherInfoToXML(&$node, &$dom)
    protected function writeOtherInfoToXML(DOMElement &$node, DOMDocument &$dom) {
	return $node;
    }

    // {{{ writeChildrenToXML(DOMDocument &$dom)
    protected function writeChildrenToXML(DOMDocument &$dom) {
	$children = $dom->createElement("children");
	foreach ($this->child_hashes as $hash) {
	    $node = $dom->createElement("node");
	    $node->setAttribute("type", "reference");
	    $node->setAttribute("hash", $hash);
	    if (false !== array_search($hash, $this->defaults))
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

    // {{{ Empty Extendable Functions
    // {{{ XMLParse(&$node)
    /**
     * This is called on each sub-node of the XMLNode this object is built from
     * If an extended class needs to parse information differently than how
     * this base class does it, override this function.
     */
    public function XMLParse(DOMElement &$node) {}
    // }}}
    // {{{ changesetXMLParse(&$node)
    /**
     * This is called on each sub-node of the XMLNode in updateFromChangesetXML
     * If an extended class needs to parse information differently than how
     * this base class does it, override this function.
     */
    public function changesetXMLParse(DOMElement &$node) {}
    // }}}

    // {{{ display()
    /**
     * This function is called when the node is asked to display itself. There
     * is no default action.
     */
    public function display() {}
    // }}}

    // {{{ displayHREF()
    /**
     * This function is called when the node is asked to specify a link to
     * itself. There is no default action.
     */
    public function displayHREF() {}
    // }}}
    // }}}

    // }}}

    public function crawl_defaults(&$defaults = array(), &$alternates = null) {
	if (is_array($alternates)) {
	    foreach ($this->child_hashes as $hash) {
		if ($this->isDefault($hash)) continue;
		$node = WordTrailsGlobal::getNode($hash);
		if (!is_object($node)) continue;
		if ($node->getType() != "node") continue;
		array_push($alternates,$hash);
	    }
	}
	foreach ($this->defaults as $hash) {
	    if (!in_array($hash, $defaults)) {
		array_push($defaults,$hash);
		$node = WordTrailsGlobal::getNode($hash);
		if (!is_object($node)) continue;
		$node->crawl_defaults($defaults, $alternates);
	    }
	}
	//return array($defaults, $alternates);
    }

    // {{{ Sorting functions

    public function sort_children($sortOn = "nID", $default_first = false) {
	$this->child_hashes = array_map(array("WordTrailsGlobal","getNodeHashFromID"), $this->child_hashes);
	if (method_exists("WordTrailsNode", "nsort_" .$sortOn))
	    usort($this->child_hashes, array("WordTrailsNode", "nsort_" .$sortOn));
	else
	    usort($this->child_hashes, array("WordTrailsNode", "nsort_nID"));
	if ($default_first) {
	    $this->child_hashes = array_merge($this->defaults, array_diff($this->child_hashes, $this->defaults));
	}
    }

    static function nsort_nID($a, $b) {
        $am = WordTrailsGlobal::getNode($a);
        $bm = WordTrailsGlobal::getNode($b);
	if (!is_object($am)) return -1;
	if (!is_object($bm)) return 1;
	$amid = $am->getNodeID();
	$bmid = $bm->getNodeID();
        if ($amid == $bmid)
            return 0;
        return ($amid < $bmid) ? -1 : 1;
    }
    static function nsort_nID_r($b, $a) {
        $am = WordTrailsGlobal::getNode($a);
        $bm = WordTrailsGlobal::getNode($b);
	if (!is_object($am)) return 1;
	if (!is_object($bm)) return -1;
	$amid = $am->getNodeID();
	$bmid = $bm->getNodeID();
        if ($amid == $bmid)
            return 0;
        return ($amid < $bmid) ? -1 : 1;
    }

    static function nsort_type($a, $b) {
        $am = WordTrailsGlobal::getNode($a);
        $bm = WordTrailsGlobal::getNode($b);
	if (!is_object($am)) return -1;
	if (!is_object($bm)) return 1;
	$amt = $am->getType();
	$bmt = $bm->getType();
        if ($amt == $bmt)
            return 0;
        return ($amt < $bmt) ? -1 : 1;
    }
    static function nsort_type_r($b, $a) {
        $am = WordTrailsGlobal::getNode($a);
        $bm = WordTrailsGlobal::getNode($b);
	if (!is_object($am)) return 1;
	if (!is_object($bm)) return -1;
	$amt = $am->getType();
	$bmt = $bm->getType();
        if ($amt == $bmt)
            return 0;
        return ($amt < $bmt) ? -1 : 1;
    }

    static function nsort_name($a, $b) {
        $am = WordTrailsGlobal::getNode($a);
        $bm = WordTrailsGlobal::getNode($b);
	if (!is_object($am)) return -1;
	if (!is_object($bm)) return 1;
        return strcasecmp($am->getName(), $bm->getName());
    }
    static function nsort_name_r($b, $a) {
        $am = WordTrailsGlobal::getNode($a);
        $bm = WordTrailsGlobal::getNode($b);
	if (!is_object($am)) return 1;
	if (!is_object($bm)) return -1;
        return strcasecmp($am->getName(), $bm->getName());
    }

    static function nsort_created($a, $b) {
        $am = WordTrailsGlobal::getNode($a);
        $bm = WordTrailsGlobal::getNode($b);
	if (!is_object($am)) return -1;
	if (!is_object($bm)) return 1;
	$amc = $am->getCreated();
	$bmc = $bm->getCreated();
        if ($amc == $bmc)
            return 0;
        return ($amc < $bmc) ? -1 : 1;
    }
    static function nsort_created_r($b, $a) {
        $am = WordTrailsGlobal::getNode($a);
        $bm = WordTrailsGlobal::getNode($b);
	if (!is_object($am)) return 1;
	if (!is_object($bm)) return -1;
	$amc = $am->getCreated();
	$bmc = $bm->getCreated();
        if ($amc == $bmc)
            return 0;
        return ($amc < $bmc) ? -1 : 1;
    }

    static function nsort_modified($a, $b) {
        $am = WordTrailsGlobal::getNode($a);
        $bm = WordTrailsGlobal::getNode($b);
	if (!is_object($am)) return -1;
	if (!is_object($bm)) return 1;
	$amm = $am->getModified();
	$bmm = $bm->getModified();
        if ($amm == $bmm)
            return 0;
        return ($amm < $bmm) ? -1 : 1;
    }
    static function nsort_modified_r($b, $a) {
        $am = WordTrailsGlobal::getNode($a);
        $bm = WordTrailsGlobal::getNode($b);
	if (!is_object($am)) return 1;
	if (!is_object($bm)) return -1;
	$amm = $am->getModified();
	$bmm = $bm->getModified();
        if ($amm == $bmm)
            return 0;
        return ($amm < $bmm) ? -1 : 1;
    }

    // }}}
}
// }}}
?>