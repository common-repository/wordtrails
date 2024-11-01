<?php
require_once "WordTrailsGlobal.inc";

class WordTrailsWidget {
    public $PostID;

    protected $nodes = array();  //node_hash => {parents:array(hash=>array('name'=>name, 'href'=>href)), children:array(hash=>array('name'=>name, 'href'=>href, 'default'=>bool))}
    protected $trails = array(); //trail_hash => array(node_hash, node_hash)

    protected $before_widget;
    protected $before_title;
    protected $after_title;
    protected $after_widget;

    public $current_trail = null;

    public function __construct($pid, $args) {
        $this->PostID = (int)$pid;
        if (!is_null($args)) {
            extract($args);
            $this->before_widget = $before_widget;
            $this->before_title = $before_title;
            $this->after_title = $after_title;
            $this->after_widget = $after_widget;
        }
    }

    public function __sleep() {
        return null;
        return array_keys(get_object_vars($this));
    }

    public function activeNodes() {
        $hashes = WordTrailsGlobal::hashesFromPID($this->PostID);
        if (empty($hashes)) return false;
        return true;
    }

    protected function updateNodes() {
        global $wpdb;
        $sql = "SELECT Hash FROM " . WordTrailsGlobal::$tables->node . " WHERE isInternal = 1 AND Type = \"node\" AND ReferenceID = %d";
        $hashes = $wpdb->get_col($wpdb->prepare($sql, $this->PostID));
        $merge = array();
        $diff = array();
        WordTrailsGlobal::makeInstance();
        foreach (array_keys(WordTrailsGlobal::$instance->nodes) as $hash) {
            $node = WordTrailsGlobal::getNode($hash, false);
            if (is_object($node)) {
                if (isset($node->PostID)) {
                    if ($node->PostID == $this->PostID) {
                        if ($node->delete_on_save) {
                            $diff[] = $hash;
                        } else {
                            $merge[] = $hash;
                        }
                    }
                }
            }
        }
        $hashes = array_unique(array_diff(array_merge($hashes,$merge), $diff));
        if (empty($hashes)) return false;

        $parents_sql = "SELECT n.Hash FROM " . WordTrailsGlobal::$tables->node . " as n LEFT JOIN " . WordTrailsGlobal::$tables->node_rel . " as nr ON nr.ParentID = n.NodeID WHERE n.Type != \"trail\" AND nr.ChildID = %d";
        foreach ($hashes as $hash) {
            $this->nodes[$hash] = (object)array("parents"=>array(), "children"=>array());
            $node = WordTrailsGlobal::getNode($hash);
            if (!is_array($this->trails[$node->trail_hash])) $this->trails[$node->trail_hash] = array();
            array_push($this->trails[$node->trail_hash], $hash);
            if (is_array($node->child_hashes) && !empty($node->child_hashes)) {
                foreach ($node->child_hashes as $chash) {
                    $this->nodes[$hash]->children[$chash] = array('default'=>$node->isDefault($chash), 'comment'=> $node->rel_comments[$chash], 'control'=> $node->rel_controls[$chash]);
                }
            }
            $parents = $wpdb->get_col($wpdb->prepare($parents_sql, WordTrailsGlobal::getNodeIDFromHash($hash)));
            if (is_array($parents) && !empty($parents)) {
                foreach ($parents as $phash) {
                    $parent = WordTrailsGlobal::getNode($phash);
                    if (!is_object($parent)) continue;
                    $this->nodes[$hash]->parents[$phash] = array('comment' => $parent->rel_comments[$hash], 'control' => $parent->rel_controls[$hash]);
                }
            }
            if (isset(WordTrailsGlobal::$instance->unsaved_parents[$hash])) {
                foreach (WordTrailsGlobal::$instance->unsaved_parents[$hash] as $phash) {
                    $parent = WordTrailsGlobal::getNode($phash, false);
                    if (!is_object($parent)) continue;
                    if ($parent->getType() == "trail") {
                        continue;
                    }
                    $this->nodes[$hash]->parents[$phash] = array('comment' => $parent->rel_comments[$hash], 'control' => $parent->rel_controls[$hash]);
                }
            }
        }

        $info_hashes = array();
        foreach (array_keys($this->nodes) as $nhash) {
            $info_hashes = array_unique(array_merge($info_hashes, array_keys($this->nodes[$nhash]->children), array_keys($this->nodes[$nhash]->parents)));
        }
        $info = WordTrailsGlobal::getNodeInfo($info_hashes, null, $thash);
        foreach (array_keys($this->nodes) as $nhash) {
            foreach (array_keys($this->nodes[$nhash]->children) as $chash) {
                if (!isset($info[$chash])) continue;
                foreach ($info[$chash] as $key => $val) {
                    $this->nodes[$nhash]->children[$chash][$key] = $val;
                }
            }
            foreach (array_keys($this->nodes[$nhash]->parents) as $phash) {
                if (!isset($info[$phash])) continue;
                foreach ($info[$phash] as $key => $val) {
                    $this->nodes[$nhash]->parents[$phash][$key] = $val;
                }
            }
        }
        return true;
    }

    public function display($crop = 30) {
        if (!$this->updateNodes()) return false;
        $this->current_trail = null;
        ?>
<div id="WordTrailsWormsEyeContainer"><?php
        $trail_hash = null;
        if (isset($_GET['t']) && !empty($_GET['t'])) {
            $trail_hash = WordTrailsGlobal::getNodeHashFromID($_GET['t']);
        }
        if ($slug = get_query_var('trail_slug')) {
            if (!empty($slug) && !is_null($slug)) $trail_hash = WordTrailsGlobal::getNodeHashFromSlug($slug);
        }
        if (!is_null($trail_hash)) {
            if (WordTrailsUtilities::isHash($trail_hash)) {
                if (false !== array_search($trail_hash, array_keys($this->trails))) {
                    $trail = WordTrailsGlobal::getNode($trail_hash);
                    if (is_object($trail)) {
                        $this->current_trail = $trail_hash;
                        WordTrailsGlobal::$instance->current_trail_id = $trail->getNodeID();
                        /* ?>

    <h4 class="current">Current trail:</h4><?php */
                    }
                }
            }
        }
        uksort($this->trails, array("WordTrailsUtilities", "sort_trails"));
        $trail_inc = 0;
        $hidingtrails = false;
        foreach ($this->trails as $trail_hash => $nodes) {
            $trail = WordTrailsGlobal::getNode($trail_hash);
            if (!is_object($trail)) continue;
            $is_current = ($this->current_trail == $trail_hash);
            /* if ($trail_inc == 1 && !is_null($this->current_trail)) { ?>

    <h4>Alternate trail<?php echo count($this->trails) > 2 ? "s" : ""; ?>:</h4><?php
            } */
            if ($trail_inc == 2) { ?>

    <div id="show_all_trails" class="show_hide_link"><a href="javascript:void(0);" onclick="showAllTrails();">Show More Trails</a></div>
    <div id="hidden_trails" class="hidden"><?php
                $hidingtrails = true;
            } ?>

    <div class="trail_holder"><?php
                    $pop = array();
                    $pop = $trail->gatherPopupInfo();
                    //if (count($pop) == 1) $pop = array();
                    ?>

        <div class="trail rounded<?php echo (!empty($pop) ? " tipthis\" title=\"" . htmlentities(json_encode($pop), ENT_QUOTES) : ""); ?>">
            <div class="float_icon">
                <img src="<?php echo WordTrailsGlobal::$urlpath; ?>imgs/<?php echo $is_current ? "curr_" : ""; ?>trail.png" alt="Trail Icon" />
            </div>
            <div class="full_bottom"><?php
                if (!is_null($trail->getName())) {?>

                <div class="trail_title"><?php echo $trail->getName(); ?></div><?php
                }
                if (!is_null($trail->getShortDesc())) {?>

                <div class="trail_caption"><?php echo $trail->getShortDesc(); ?></div><?php
                } ?>

            </div>
        </div><?php
            foreach ($nodes as $node_hash) {
                $node = WordTrailsGlobal::getNode($node_hash);
                if (is_object($node)) {
                    $pop = array();
                    $pop = $node->gatherPopupInfo();
                    //if (count($pop) == 1) $pop = array();
                    ?>

        <div class="here_wrapper">
        <div class="here rounded<?php echo (!empty($pop) ? " tipthis\" title=\"" . htmlentities(json_encode($pop), ENT_QUOTES) : ""); ?>">
            <div class="float_icon">
                <img src="<?php echo WordTrailsGlobal::$urlpath; ?>imgs/<?php echo $is_current ? "curr" : "intrsctng"; ?>_node.png" alt="<?php echo $is_current ? "You Are Here" : "Intersection"; ?>" />
            </div>
            <div class="full_bottom"><?php
                    if (!is_null($node->getName())) { ?>

                <div class="here_title"><?php echo $node->getName(); ?></div><?php
                    }
                    if (!is_null($node->getShortDesc())) { ?>

                <div class="here_caption"><?php echo $node->getShortDesc(); ?></div><?php
                    } ?>

            </div>
        </div>
        </div>
        <div class="child_box"><?php
                    if (empty($this->nodes[$node_hash]->children)) { ?>

            <div class="node_box"><i>[No next]</i></div><?php
                    } else {
                        uasort($this->nodes[$node_hash]->children, array("WordTrailsUtilities", "sort_default_children"));
                        foreach ($this->nodes[$node_hash]->children as $child_hash => $info) {
                            if (empty($info["name"]) || empty($info["href"])) continue;
                            $href = $info["href"];
                            $name = trim($info["name"]);
                            $next = (strlen($info["comment"]) > 0 && strpos($info["comment"], "|") !== false) ? explode("|", $info["comment"]) : ($info["default"] ? "Next" : "Alt. Next");
                            if (is_array($next)) $next = $next[1];
                            $arrow = WordTrailsGlobal::$controls->getNextArrow($info["control"], $info["default"]);
                            $pop = array();
                            $child = WordTrailsGlobal::getNode($child_hash);
                            if (is_object($child)) {
                                $pop = $child->gatherPopupInfo();
                                //if (count($pop) == 1) $pop = array();
                            }
                            ?>

            <div class="node_box" onclick="document.location.href='<?php echo $href; ?>'">
                <div class="node rounded<?php echo (!empty($pop) ? " tipthis\" title=\"" . htmlentities(json_encode($pop), ENT_QUOTES) : ""); ?>">
                    <div class="node_rel">
                        <div class="node_rel_comment left"><?php echo $next; ?></div>
                        <div class="node_rel_img right">
                            <img src="<?php echo WordTrailsGlobal::$urlpath . "imgs/arrows/{$arrow}";?>" alt="<?php echo $next; ?>" />
                        </div>
                        <br class="clear" />
                    </div>
                    <div class="node_title"><?php echo $name; ?></div>
                </div>
            </div><?php
                        }
                    } ?>

        </div>
        <div class="parent_box"><?php
                    if (empty($this->nodes[$node_hash]->parents)) { ?>

            <div class="node_box"><i>[No previous]</i></div><?php
                    } else {
                        foreach ($this->nodes[$node_hash]->parents as $parent_hash => $info) {
                            if (empty($info["name"]) || empty($info["href"])) continue;
                            $href = $info["href"];
                            $name = trim($info["name"]);
                            $next = (strlen($info["comment"]) > 0 && strpos($info["comment"], "|") !== false) ? explode("|", $info["comment"]) : "Previous";
                            if (is_array($next)) $next = $next[1];
                            $arrow = WordTrailsGlobal::$controls->getPreviousArrow($info["control"]);
                            $pop = array();
                            $parent = WordTrailsGlobal::getNode($parent_hash);
                            if (is_object($parent)) {
                                $pop = $parent->gatherPopupInfo();
                                //if (count($pop) == 1) $pop = array();
                            }
                            ?>

            <div class="node_box" onclick="document.location.href='<?php echo $href; ?>'">
                <div class="node rounded<?php echo (!empty($pop) ? " tipthis\" title=\"" . htmlentities(json_encode($pop), ENT_QUOTES) : ""); ?>">
                    <div class="node_rel">
                        <div class="node_rel_comment right"><?php echo $next; ?></div>
                        <div class="node_rel_img left">
                            <img src="<?php echo WordTrailsGlobal::$urlpath . "imgs/arrows/{$arrow}";?>" alt="<?php echo $next; ?>" />
                        </div>
                        <br class="clear" />
                    </div>
                    <div class="node_title"><?php echo $name; ?></div>
                </div>
            </div><?php
                        }
                    } ?>

        </div>
        <br class="clear" /><?php
                }
            }?>

        <div class="trail_links">
            <a href="http://trailmeme.com/" class="ltoh"><img src="<?php echo WordTrailsGlobal::$urlpath; ?>imgs/buttons/trailmeme_l.png" alt="TrailMeme" title="Global Trails Index" /></a><?php
        if (current_user_can(WordTrailsGlobal::CAP_SAVE)) { ?>

            <a href="<?php echo WordTrailsGlobal::$instance->menu->href_to_edit_node($trail_hash); ?>" class="ltoh"><img src="<?php echo WordTrailsGlobal::$urlpath; ?>imgs/buttons/edit_l.png" alt="Edit" title="Edit Trail" /></a>
        <?php } ?>

            <a href="<?php echo WordTrailsUtilities::appendVar(WordTrailsUtilities::redirect_link_to("print", WordTrailsGlobal::getNodeIDFromHash($trail_hash)), "button", 1, true); ?>" rel="external" class="ltoh"><img src="<?php echo WordTrailsGlobal::$urlpath; ?>imgs/buttons/print_l.png" alt="Print" title="Print Trail" /></a><?php
        if (!is_null(WordTrailsGlobal::get_option(WordTrailsGlobal::TRAILS_PAGE_OPTION))) { ?>
            <a href="<?php echo $trail->displayHREF(); ?>" class="ltoh"><img src="<?php echo WordTrailsGlobal::$urlpath; ?>imgs/buttons/view_map_l.png" alt="View Map" title="View Trail Map" /></a>
        <?php } ?></div>
    </div><?php
            $trail_inc++;
        }
        if ($hidingtrails) { ?>

    <div id="hide_extra_trails" class="show_hide_link"><a href="javascript:void(0);" onclick="hideExtraTrails();">Show Fewer Trails</a></div>
    </div><?php
        }
        ?>

</div>
<div style="height: 10px;">&nbsp;</div><?php
    }

    public function blazing() {
        if (isset($_POST["pid"]) && $_POST["trl_blz"] == "1") WordTrailsGlobal::blazeTrail();
        $trails = WordTrailsGlobal::getAllTrailNodes("Name");

        $blazing_default = (!is_null(WordTrailsGlobal::$instance->blazing_trail_id) && false === array_search(WordTrailsGlobal::getNodeHashFromID(WordTrailsGlobal::$instance->blazing_trail_id), array_keys($this->trails)));

        ?>

<div id="show_trail_blazing" class="show_hide_link<?php if ($blazing_default) echo ' hidden';?>"><a href="javascript:void(0);" onclick="showTrailBlazing();">Show Trail Blazing Tools</a></div>
<div id="WordTrailsBlazingContainer"<?php if (!$blazing_default) echo ' class="hidden"'; ?>>
    <strong>Add this post to:</strong>
    <form action="<?php echo str_replace( '%7E', '~', $_SERVER['REQUEST_URI']); ?>" id="trail_blazing_form" method="post">
    <div>
    <input type="hidden" name="pid" value="<?php echo $this->PostID; ?>" />
    <input type="hidden" name="trl_blz" value="1" />
    <select id="blaze_select" name="blaze_trail" onkeyup="blazeSelect();" onchange="blazeSelect();">
        <option<?php if (!$blazing_default) echo ' selected="selected"'; ?> value="null" style="padding-bottom:5px">-----Select a Trail-----</option><?php
        foreach ($trails["live"] as $trail) {
            if (false !== array_search($trail->getHash(), array_keys($this->trails))) continue; ?>

        <option value="<?php echo $trail->getHash(); ?>" title="<?php echo $trail->getName(); ?>"<?php if ($blazing_default && $trail->getNodeID() == WordTrailsGlobal::$instance->blazing_trail_id) echo ' selected="selected"'; ?>><?php echo WordTrailsUtilities::truncateLongName($trail->getName()); ?></option><?php
        }
        foreach ($trails["db"] as $trail) {
            if (false !== array_search($trail->Hash, array_keys($this->trails))) continue; ?>

        <option value="<?php echo $trail->Hash; ?>" title="<?php echo $trail->Name; ?>"<?php if ($blazing_default && $trail->NodeID == WordTrailsGlobal::$instance->blazing_trail_id) echo ' selected="selected"'; ?>><?php echo WordTrailsUtilities::truncateLongName($trail->Name); ?></option><?php
        }?>

        <option value="new_trail" style="padding-top:5px">Make New Trail &rarr;</option>
    </select>
    <fieldset id="new_child_options" style="border: none;"<?php if(!$blazing_default) echo ' class="hidden"'; ?>>
        <label><input type="radio" name="new_child_radio" value="as_child"<?php if (!WordTrailsGlobal::$instance->blazing_default_norel) echo ' checked="checked"'; ?> /> Add after last node added</label><br />
        <label><input type="radio" name="new_child_radio" value="no_rels"<?php if (WordTrailsGlobal::$instance->blazing_default_norel) echo ' checked="checked"'; ?> /> Add with no relationships</label>
    </fieldset>
    <fieldset id="new_trail_options" style="border: none;" class="hidden">
        <label>New Trail Name: <input type="text" name="new_trail_name" value="" /></label>
    </fieldset>
    <input type="submit" id="blaze_submit" value="Add Post"<?php if(!$blazing_default) echo ' class="hidden"'; ?> />
    </div>
    </form>
    <?php if ($this->activeNodes()) { ?>
    <br />
    <strong>Remove this post from:</strong>
    <form id="remove_from_trail_form" action="<?php echo str_replace( '%7E', '~', $_SERVER['REQUEST_URI']); ?>" method="post">
    <div>
    <input type="hidden" name="pid" value="<?php echo $this->PostID; ?>" />
    <input type="hidden" name="trl_blz" value="1" />
    <input type="hidden" id="remove_from_trail" name="remove_from_trail" value="true" />
    <select id="remove_select" name="remove_trail" onkeyup="removeSelect();" onchange="removeSelect();" width="100%">
        <option selected="selected" value="null" style="padding-bottom:5px;">-----Select a Trail-----</option><?php
        foreach ($trails["live"] as $trail) {
            if (false === array_search ($trail->getHash(), array_keys($this->trails))) continue; ?>

            <option value="<?php echo $trail->getHash(); ?>" title="<?php echo $trail->getName(); ?>"><?php echo WordTrailsUtilities::truncateLongName($trail->getName()); ?></option><?php
        }
        foreach ($trails["db"] as $trail) {
            if (false === array_search ($trail->Hash, array_keys($this->trails))) continue; ?>

            <option value="<?php echo $trail->Hash; ?>" title="<?php echo $trail->Name; ?>"><?php echo WordTrailsUtilities::truncateLongName($trail->Name); ?></option><?php
        } ?>
    </select>
    <input type="submit" id="remove_submit" class="hidden" value="Remove Post" />
    </div>
    </form>
    <?php } ?>
    <div id="hide_trail_blazing" class="show_hide_link"><a href="javascript:void(0);" onclick="hideTrailBlazing();">Hide Trail Blazing Tools</a></div>
</div><?php
    }

    public function displayWidget() {
        if ((!is_single() && !is_page()) || (is_page(WordTrailsGlobal::get_option(WordTrailsGlobal::TRAILS_PAGE_OPTION)))) return;
        //if (!$this->activeNodes() && !(current_user_can(WordTrailsGlobal::CAP_MOD) && (is_single()||is_page()) && !is_page(WordTrailsGlobal::get_option(WordTrailsGlobal::TRAILS_PAGE_OPTION)))) return;
        echo $this->before_widget;
        echo $this->before_title;
        //Add config option
        echo "Trails";
        echo $this->after_title;
        echo "<div id=\"WordTrailsWidget\">";
        if ($this->activeNodes())
            $this->display();
        //if (current_user_can(WordTrailsGlobal::CAP_MOD) && (is_single()||is_page()) && !is_page(WordTrailsGlobal::get_option(WordTrailsGlobal::TRAILS_PAGE_OPTION)))
            $this->blazing();
        echo "</div>";
        echo $this->after_widget;
    }
    
    
    public static function widget_options() {
        WordTrailsUtilities::setStatics();
        if (!empty($_POST[WordTrailsGlobal::WIDGET_TITLE_OPTION])) WordTrailsGlobal::update_option(WordTrailsGlobal::WIDGET_TITLE_OPTION, $_POST[WordTrailsGlobal::WIDGET_TITLE_OPTION]);
        echo "<label>Title:<br />";
        echo "<input name=\"" . WordTrailsGlobal::WIDGET_TITLE_OPTION . "\" type=\"text\" value=\"" . WordTrailsGlobal::get_option(WordTrailsGlobal::WIDGET_TITLE_OPTION) . "\" /></label>";
        echo "<br /><strong>Note:</strong> This widget will not display on any page that shows multiple posts, or on the Trail Index page.";
    }

}

?>