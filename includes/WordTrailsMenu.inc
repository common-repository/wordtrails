<?php
/**
 * This is the Menu class
 *
 * This file contains the WordTrailsMenu class, the one class to rule them all.
 *
 * @package WordTrails
 * @since 0.2.0
 * @author Jesse Silverstein <jesse.silverstein@xerox.com>
 * @copyright 2008 XIG: SemPrint
 * @global WordPress $wpdb WordPress database management class
 */

// {{{ WordTrailsMenu
/**
 * Menu class
 *
 * The menu class installs, displays, and processes all of the WordTrails menus
 *
 * @package WordTrails
 * @since 0.2.0
 * @author Jesse Silverstein <jesse.silverstein@xerox.com>
 * @copyright 2008 XIG: SemPrint
 * @global WordPress $wpdb WordPress database management class
 */
class WordTrailsMenu {
    // {{{ Properties/Variables
    public $topmenu = "Trails";
    protected $menus = array(
	"Edit" => array("func" => "trailsMenu", "slug" => "Edit", "cap" => WordTrailsGlobal::CAP_SAVE)
	,"Add New" => array("func" => "add_new_trail", "slug" => "Add_New", "cap" => WordTrailsGlobal::CAP_SAVE)
	,"Settings" => array("func" => "settingsMenu", "slug" => "Settings", "cap" => 'edit_plugins')
	,"Analytics" => array("func" => "analyticsMenu", "slug" => "Analytics", "cap" => WordTrailsGlobal::CAP_SAVE)
	,"More Information" => array("func"  => "helpMenu", "slug" => "Info", "cap" => WordTrailsGlobal::CAP_SAVE)
    );
    protected $awake = true;
    protected $modes = array(
	"edit" => "edit_node"
	,"trail_analytics" => "single_trail_analytics"
	,"sid_analytics" => "single_sid_analytics"
    );
    protected $messages = array();
    
    const TRAIL_PAGE_KEY = "trail_page";
    const TRAIL_PAGE_MSG = "You must provide a <a href=\"%s\">page to display Trails</a> for it to work.";
    const TRAIL_PAGE_TITLE = "Trailmeme for WordPress is almost ready.";
    const TRAIL_PAGE_CAP = 'edit_plugins';
    
    const NO_SIDEBAR_KEY = "no_sidebar";
    const NO_SIDEBAR_MSG = "Please consider using a widget-friendly <a href=\"%s\">theme</a>.";
    const NO_SIDEBAR_TITLE = "Trailmeme for WordPress is lacking features.";
    const NO_SIDEBAR_CAP = 'switch_themes';
    
    const WIDGET_KEY = "WEV_widget";
    const WIDGET_MSG = "Include the Trails Navigation Widget in a <a href=\"%s\">sidebar</a> (requires widget-friendly theme)";
    const WIDGET_TITLE = "Trailmeme for WordPress is lacking features.";
    const WIDGET_CAP = 'switch_themes';
    
    const TRAILMEME_KEY = "TrailMeme";
    const TRAILMEME_MSG = "to index your trails in the Trailmeme search engine. More great features to come!";
    const TRAILMEME_TITLE = "<a href=\"%s\">Register</a> your plugin with trailmeme.com";
    const TRAILMEME_CAP = 'edit_plugins';

    public $option_num_per_page = array(5, 10, 15, 20, 50);
    // }}}

    public function __sleep() {
	$this->awake = false;
	//$this->messages = array();
	return array_diff(array_keys(get_object_vars($this)), array("menus"));
    }
    public function __wakeup() {
	$this->awake = true;
    }
    public function __toString() {
	return "WordTrails Menu object";
    }
    
    public function isMenuPage($menu) {
	if (!isset($_GET["page"])) return false; //not possible, but ... yeah
	if (!isset($this->menus[$menu])) return false; //we don't have one of those, kthx
	if ($_GET["page"] == $this->menus[$menu]["slug"]) return true;
	return false;
    }
    
    public function __call($name, $arguments) {
	if (isset($this->messages[$name])) {
	    echo "\n";
	    echo $this->messages[$name];
	    echo "\n";
	}
    }

    // {{{ installMenus()
    public function installMenus() {
	$this->continueMenu = true;
	$this->flash_change = array();
	$this->processPOST();
	$page = add_menu_page($this->topmenu, $this->topmenu, $this->menus["Edit"]["cap"], $this->topmenu, array("WordTrailsUtilities", $this->menus["Edit"]["func"]), WordTrailsGlobal::$urlpath . "imgs/logos/tmicon.png");
	add_action("admin_print_scripts-$page", array("WordTrailsUtilities", "menu_head"));
	$first = true;
	$_page = "_page_";
	foreach ($this->menus as $title => $menu) {
	    /*
	    if ($title == "Analytics") {
		if (0 == WordTrailsGlobal::get_option(WordTrailsGlobal::ANALYTICS_COLLECT_OPTION)) continue;
	    }
	    */
	    $page = add_submenu_page($this->topmenu, $this->topmenu . ": " . $title, $title, $menu["cap"], ($first ? $this->topmenu : $menu["slug"]), array("WordTrailsUtilities", $menu["func"]));
	    if ($page) add_action("admin_print_scripts-$page", array("WordTrailsUtilities", "menu_head"));
	    $this->menus[$title]["slug"] = substr($page, strpos($page, $_page) + strlen($_page));
	    $first = false;
	}
	$settings_hook = $this->menus["Settings"]["slug"];
	if (WordTrailsUtilities::warnTrailPage() && current_user_can(self::TRAIL_PAGE_CAP)) {
	    $this->addMessage(sprintf(self::TRAIL_PAGE_MSG, admin_url("admin.php?page={$settings_hook}")), self::TRAIL_PAGE_KEY, self::TRAIL_PAGE_TITLE, true, WordTrailsGlobal::WARN_TRAIL_PAGE);
	} elseif (WordTrailsUtilities::warnNoSidebar() && current_user_can(self::NO_SIDEBAR_CAP)) {
	    $this->addMessage(sprintf(self::NO_SIDEBAR_MSG, admin_url("themes.php")), self::NO_SIDEBAR_KEY, self::NO_SIDEBAR_TITLE, false, WordTrailsGlobal::WARN_SIDEBAR);
	} elseif (WordTrailsUtilities::warnWidget() && current_user_can(self::WIDGET_CAP)) {
	    $this->addMessage(sprintf(self::WIDGET_MSG, admin_url("widgets.php")), self::WIDGET_KEY, self::WIDGET_TITLE, false, WordTrailsGlobal::WARN_WIDGET);
	} elseif (WordTrailsUtilities::warnTrailMeme() && current_user_can(self::TRAILMEME_CAP)) {
	    $this->addMessage(self::TRAILMEME_MSG, self::TRAILMEME_KEY, sprintf(self::TRAILMEME_TITLE, admin_url("admin.php?page={$settings_hook}")), false, WordTrailsGlobal::WARN_TRAILMEME);
	}

    }

    public function insertCSS() { ?>
<link rel="stylesheet" type="text/css" href="<?php echo WordTrailsGlobal::$urlpath; ?>css/menu.css" />
<?php
    }
    // }}}
    
    public function nextMessage() {
	$settings_hook = $this->menus["Settings"]["slug"];
	if (WordTrailsUtilities::warnTrailPage() && current_user_can(self::TRAIL_PAGE_CAP)) {
	    return $this->getMessage(sprintf(self::TRAIL_PAGE_MSG, admin_url("admin.php?page={$settings_hook}")), self::TRAIL_PAGE_KEY, self::TRAIL_PAGE_TITLE, true, WordTrailsGlobal::WARN_TRAIL_PAGE);
	} elseif (WordTrailsUtilities::warnNoSidebar() && current_user_can(self::NO_SIDEBAR_CAP)) {
	    return $this->getMessage(sprintf(self::NO_SIDEBAR_MSG, admin_url("themes.php")), self::NO_SIDEBAR_KEY, self::NO_SIDEBAR_TITLE, false, WordTrailsGlobal::WARN_SIDEBAR);
	} elseif (WordTrailsUtilities::warnWidget() && current_user_can(self::WIDGET_CAP)) {
	    return $this->getMessage(sprintf(self::WIDGET_MSG, admin_url("widgets.php")), self::WIDGET_KEY, self::WIDGET_TITLE, false, WordTrailsGlobal::WARN_WIDGET);
	} elseif (WordTrailsUtilities::warnTrailMeme() && current_user_can(self::TRAILMEME_CAP)) {
	    return $this->getMessage(self::TRAILMEME_MSG, self::TRAILMEME_KEY, sprintf(self::TRAILMEME_TITLE, admin_url("admin.php?page={$settings_hook}")), false, WordTrailsGlobal::WARN_TRAILMEME);
	} else return "";
    }

    public function printMessages() {
	return false;
	if (count($this->messages) > 0) {
	    foreach ($this->messages as $message) {
		echo "<br style=\"clear:both;\" /><div id=\"message\" class=\"updated fade\"><p>$message</p></div>";
	    }
	}
    }
    
    public function addMessage($message, $key, $title = "", $error = false, $disable_option = null) {
	if (!is_null($disable_option)) {
	    wp_enqueue_script("wt_warnings", WordTrailsGlobal::$urlpath . "js/warning.js", array('jquery'));
	    WordTrailsUtilities::requireAjaxURL();
	}
	$this->messages[$key] = $this->getMessage($message, $key, $title, $error, $disable_option);
	add_action("admin_notices", array($this, $key));
    }
    
    public function getMessage($message, $key, $title = "", $error = false, $disable_option = null) {
	$before = "<div id=\"message-{$key}\" class=\"" . ($error ? "error" : "updated fade") . "\"><p style=\"float:left\">";
	$after = "</p><br clear=\"all\" /></div>";
	if (!empty($title)) $title = "<strong>{$title}</strong> ";
	$disable = "";
	if (!is_null($disable_option)) {
	    $disable = "<div id=\"disable_warning\" style=\"float:right; text-align:right; line-height:1; font-size:8pt;\">";
	    $disable .= "<a href=\"#disable\" onclick=\"disableWarning('{$disable_option}');\">Do not show again</a>";
	    $disable .= " <a href=\"#disable\" onclick=\"disableWarning('{$disable_option}');\"><img src=\"" . WordTrailsGlobal::$urlpath . "imgs/buttons/close_icon.gif\" alt=\"Close\" /></a>";
	    //$disable .= "<br /><span style=\"font-size:7pt;color:#555;font-style:italic;\">(reset on settings page)</span>";
	    $disable .= "</div>";
	}
	return $before . $title . $message . $disable . $after;
    }


    public function processPOST() {
	if (!isset($_POST['process']) || ! is_array($_POST['process'])) return 0;
	$ret = array();
	//superdump($_POST, true);
	foreach ($_POST['process'] as $process) {
	    if (!is_array($process) || empty($process['func']) || empty($process['args'])) continue;
	    $ret[] = $this->$process['func']($process['args']);
	    if (!$this->continueMenu) break;
	}
	return $ret;
    }

    public function href_to_edit_node($hash, $vars = array()) {
	$rtn =  "admin.php?page=" . $this->topmenu . "&amp;mode=edit&amp;hash=$hash";
	$vars = array_diff($vars, array("page", "edit", "hash"));
	foreach ($vars as $var) {
	    if (isset($_GET[$var]))
		$rtn .= "&amp;" . $var . "=" . $_GET[$var];
	}
	return admin_url($rtn);
    }
    public function href_to_submit($hash = null) {
	if ($_GET["page"] == $this->menus["Add New"]["slug"] && !is_null($hash)) return str_replace("&amp;", "&", $this->href_to_edit_node($hash));
	return str_replace( '%7E', '~', $_SERVER['REQUEST_URI']);
    }
    public function build_href($newvars = array(), $trash_vars = array()) {
	$allvars = array_diff(array_unique(array_merge(array_keys($_REQUEST), array_keys((array)$newvars))), array("page", "reverse"), (array)$trash_vars);
	$href = "admin.php?page=";
	if (isset($newvars["page"])) $href .= $newvars["page"];
	else $href .= $_REQUEST["page"];
	foreach ($allvars as $var) {
	    if (isset($newvars[$var])) $href .= "&amp;{$var}={$newvars[$var]}";
	    elseif (isset($_REQUEST[$var]) && !empty($_REQUEST[$var])) $href .= "&amp;{$var}={$_REQUEST[$var]}";
	}
	return admin_url($href);
    }

    public function trailsMenu() {
	//$this->continueMenu = true;
	global $wpdb;
	//$this->processPOST();
	if (isset($_GET["mode"])) {
	    if (false !== array_search($_GET["mode"], array_keys($this->modes))) {
		call_user_func(array($this, $this->modes[$_GET["mode"]]));
	    }
	}
	if (!$this->continueMenu) return;
	$trails = WordTrailsGlobal::getAllTrailNodes("Name, ShortDesc");
	$tpp = "trails_per_page";
	$tp = "trails_paged";
	$total = (count($trails["db"])+count($trails["live"]));
	$maxcount = (isset($_GET[$tpp]) && (int)$_GET[$tpp]>0 ) ? (int)$_GET[$tpp]:10;
	$page = (isset($_GET[$tp]) && (int)$_GET[$tp]>0) ? (int)$_GET[$tp]:1;
	$start = ($page-1)*$maxcount;
	while ($start > $total) {
	    $start -= $maxcount;
	    $page--;
	}
	$page_links = paginate_links( array(
	    'base' => add_query_arg( $tp, '%#%' ),
	    'format' => '',
	    'total' => ceil($total/$maxcount),
	    'current' => $page
	));

	?>

<div class="wrap">
    <br class="clear" />
    <h2><?php _e( 'Edit Trails', WordTrailsGlobal::DOMAIN ); ?></h2><?php
	//$this->printMessages();
	$this->num_per_page($tpp, 10, $tp);
	if ($page_links) { ?>
    <br class="clear" />
    <div class="tablenav">
	<div class='tablenav-pages'><?php echo $page_links; ?></div>
    </div><?php
	} ?>

    <br class="clear" />
    <table class="widefat" cellspacing="4">
    <thead>
    <tr>
        <th scope="col"><?php _e( 'Trail Name', WordTrailsGlobal::DOMAIN ); ?></th>
        <th scope="col"><?php _e( 'Caption', WordTrailsGlobal::DOMAIN ); ?></th>
        <th scope="col" class="num"><?php _e( 'Number of Markers', WordTrailsGlobal::DOMAIN ); ?></th><?php if(0) { ?>
        <th scope="col" class="num"><?php _e( 'Unsaved Changes', WordTrailsGlobal::DOMAIN ); ?></th><?php } ?>
        <th scope="col" style="min-width:125px;"><?php _e( 'Actions' ); ?></th>
    </tr>
    </thead>
    <tbody>
    <?php
    if (is_array($trails) && (!empty($trails["db"]) || !empty($trails["live"]))) {
	$alt = false;
	$count = 0;
	foreach ($trails["db"] as $trail) {
	    $count++;
	    if ($count <= $start) continue;
	    if ($count > ($maxcount+$start)) break;
	    echo "\n\t<tr";
            if ( $alt ) echo ' class="alternate"';
	    echo ">";
	    $alt = !$alt;
	    echo "\n\t\t<td align=\"left\">" . $trail->Name . "</td>";
	    echo "\n\t\t<td align=\"left\">" . $trail->ShortDesc . "</td>";
	    $num = (int)$wpdb->get_var($wpdb->prepare("SELECT COUNT(ChildID) FROM " . WordTrailsGlobal::$tables->node_rel . " WHERE ParentID = %d GROUP BY ParentID", $trail->NodeID));
	    ?>

	<td class="num"><?php echo $num; ?></td><?php if(0) { ?>
	<td class="num">0</td><?php } ?>
	<td align="left">
	    <form name="modify" id="form_modify_<?php echo $trail->NodeID; ?>" method="post" action="<?php echo $this->href_to_submit(); ?>">
	    <input id="form_modify_func_<?php echo $trail->NodeID; ?>" type="hidden" name="process[modify][func]" value="" />
	    <input id="form_modify_arg_hash_<?php echo $trail->NodeID; ?>" type="hidden" name="process[modify][args][hash]" value="<?php echo $trail->Hash; ?>" />
	    <input type="button" class="<?php wt_button_class(); ?>"  value="Edit" onclick="jQuery('#form_modify_func_<?php echo $trail->NodeID; ?>').attr('value','to_edit_node');jQuery('#form_modify_<?php echo $trail->NodeID; ?>').submit();" />
	    <input type="button" class="<?php wt_button_class(); ?> trail-delete"  value="Delete";" />
	    </form>
	</td>
    </tr><?php
	}
	/*jQuery('#form_modify_func_<?php echo $trail->NodeID; ?>').attr('value','delete_node');jQuery('#form_modify_<?php echo $trail->NodeID; ?>').submit();*/
	foreach ($trails["live"] as $trail) {
	    if ($trail->is_empty()) continue;
	    $count++;
	    if ($count <= $start) continue;
	    if ($count > ($maxcount+$start)) break;
	    echo "\n\t<tr";
            if ( $alt ) echo ' class="alternate"';
	    echo ">";
	    $alt = !$alt;
	    echo "\n\t\t<td align=\"left\">" . $trail->getName() . "</td>";
	    echo "\n\t\t<td align=\"left\">" . $trail->getShortDesc() . "</td>";
	    //$num = max(0,(int)$wpdb->get_var($wpdb->prepare("SELECT COUNT(ChildID) FROM " . WordTrailsGlobal::$tables->node_rel . " WHERE ParentID = %d GROUP BY ParentID", $trail->getNodeID())));
	    echo "\n\t\t<td class=\"num\">" . count($trail->child_hashes) . "</td>";
	    //echo "\n\t\t<td class=\"num\">" . count($trail->unsaved()) . "</td>";
	    ?>

	<td align="left">
	    <form name="modify" id="form_modify_<?php echo $trail->getNodeID(); ?>" method="post" action="<?php echo $this->href_to_submit(); ?>">
	    <input id="form_modify_func_<?php echo $trail->getNodeID(); ?>" type="hidden" name="process[modify][func]" value="" />
	    <input id="form_modify_arg_hash_<?php echo $trail->getNodeID(); ?>" type="hidden" name="process[modify][args][hash]" value="<?php echo $trail->getHash(); ?>" />
	    <input type="button" class="<?php wt_button_class(); ?>"  value="Edit" onclick="jQuery('#form_modify_func_<?php echo $trail->getNodeID(); ?>').attr('value','to_edit_node');jQuery('#form_modify_<?php echo $trail->getNodeID(); ?>').submit();" />
	    <input type="button" class="<?php wt_button_class(); ?> trail-delete"  value="Delete" />
	    <?php if ($trail->hasUnsaved()) { ?>

	    <input type="button" class="<?php wt_button_class(); ?>"  value="Save" onclick="jQuery('#form_modify_func_<?php echo $trail->getNodeID(); ?>').attr('value','save_node');jQuery('#form_modify_<?php echo $trail->getNodeID(); ?>').submit();" /><?php } ?>
	    </form>
	</td><?php
	    echo "\n\t</tr>";
	}
    } else {
	echo "\n\t<tr><td colspan=\"5\" align=\"center\"><i>No Trails Found</i></td></tr>";
    }
    ?>

    </form>
    </tbody>
    </table><?php 	if ($page_links) { ?>
    <br class="clear" />
    <div class="tablenav">
	<div class='tablenav-pages'><?php echo $page_links; ?></div>
    </div><?php
	} ?>
<br /><br />
</div>
    <?php
    }
    
    public function add_new_trail() {
	$trail = WordTrailsGlobal::createNewNode(true, "trail");
	if (is_object($trail)) {
	    $this->edit_node($trail->getHash());
	}
    }

    public function add_trail($args) {
	if (!is_array($args)) return false;
	extract($args);
	$trail = WordTrailsGlobal::createNewNode(true, "trail");
	if (is_object($trail)) {
	    $trail->setName($trail_name);
	    $trail->setShort($trail_desc);
	    $this->to_edit_node(array("hash"=>$trail->getHash()));
	}
    }
    public function save_node($args) {
	extract($args);
	if (!isset($hash)) return false;
	global $wpdb;
	$node = WordTrailsGlobal::getNode($hash);
	if (!is_object($node)) return false;
	$wpdb->show_errors();
	$node->save();
    }
    public function revert_node($args) {
	extract($args);
	if (!isset($hash)) return false;
	$node = WordTrailsGlobal::getNode($hash);
	if (!is_object($node)) return false;
	$node->revertToSaved();
    }
    public function to_edit_node($args) {
	extract($args);
	if (!isset($hash)) return false;
	header("Location: " . str_replace("&amp;", "&", $this->href_to_edit_node($hash)));
    }
    public function edit_node($hash = null) {
	if (isset($_GET["hash"])) {
	    $hash = $_GET["hash"];
	} elseif ($_GET["page"] != $this->menus["Add New"]["slug"]) {
	    return false;
	}
	if (is_null($hash)) return false;
	global $wpdb;
	$node = WordTrailsGlobal::getNode($hash);
	if (!is_object($node)) return false;
	$this->continueMenu = false;
	$type = $node->getType();
	$child_type = "child";
	$children_type = "children";
	if ($type == "trail") {
	    $child_type = "marker";
	    $children_type = "markers";
	}

	add_meta_box("editnodeinfo", ucwords($type) . " Info", array($this, "edit_node_info_form"), "editnode");
	add_meta_box("editnodechildren", "Manage " . ucwords($children_type), array($this, "edit_node_children_form"), "editnode");
	if (!empty($node->trail_hash) && $node->trail_hash != $hash) {
	    $trail = WordTrailsGlobal::getNode($node->trail_hash);
	    if (is_object($trail)) {
		add_meta_box("editnodeaddchildren", "Add " . ucwords($children_type) . " from \"" . $trail->getName() . "\" Trail", array($this, "edit_node_add_children_form"), "editnode");
	    }
	} else {
	    //add_meta_box("editvisualtrail", "Visually Edit Trail", array($this, "edit_visual_trail"), "editnode");
	    add_meta_box("editnodeaddpost", "Add " . ucwords($children_type) . " From Posts on This Blog", array($this, "edit_node_add_post_form"), "editnode");
	    //add_meta_box("editnodeaddurl", "Create " . ucwords($child_type) . " From an External URL", array($this, "edit_node_add_url_form"), "editnode");
	}
	//add_meta_box("editnodeaddexttrail", "Create " . ucwords($children_type) . " From Another Plugin-enabled Blog", array($this, "edit_node_add_p2p_form"), "editnode");

	?>
<div class="wrap">
<br style="clear: both;" />
<h2>
    Editing <?php echo ucwords($type); ?>: <a href="<?php echo $this->href_to_edit_node($node->getHash()); ?>"><?php
$nname = $node->getName();
if (empty($nname)) $nname = "<i>Unnamed</i>";
echo "{$nname}</a>";
?>
    <a href="<?php echo WordTrailsUtilities::redirect_link_to("xml", $hash); ?>" class="ltoh">
	<img src="<?php echo WordTrailsGlobal::$urlpath; ?>imgs/buttons/xml_l.png" alt="View XML" title="View XML" />
    </a>
</h2>
<?php
$unsaved = false;
if ($type == "node" && !is_null($node->trail_hash)) {
    $trail = WordTrailsGlobal::getNode($node->trail_hash);
    if (is_object($trail)) {
	$tname = $trail->getName();
	if ($trail->hasUnsaved()) $unsaved = true;
	if (empty($tname)) $tname = "<i>Unnamed</i>"; ?>

<br style="clear: both;" />
<h2 style="border-bottom: none;margin:0px;padding:0px;float:left;">
    <small>
	on Trail: <a href="<?php echo $this->href_to_edit_node($trail->getHash()); ?>"><?php echo $tname; ?></a>
	<a href="<?php echo WordTrailsUtilities::redirect_link_to("xml", $trail->getHash()); ?>" class="ltoh">
	    <img src="<?php echo WordTrailsGlobal::$urlpath; ?>imgs/buttons/xml_l.png" alt="View Trail XML" title="View TrailXML" />
	</a>
    </small>
</h2><?php
    }
}
?>
<br style="clear: both;" />
<div id="poststuff">

<?php
do_meta_boxes("editnode", 'advanced', $hash);
wp_nonce_field( 'closedpostboxes', 'closedpostboxesnonce', false );
wp_nonce_field( 'meta-box-order', 'meta-box-order-nonce', false );
?>

</div>
</div>
<script language="JavaScript" type="text/javascript">
<!--

    var ajax_url = "<?php bloginfo( 'wpurl' ); ?>/wp-admin/admin-ajax.php";

    <?php post_box_toggles("editnode"); ?>

    // close postboxes that should be closed
    jQuery('.if-js-closed').removeClass('if-js-closed').addClass('closed');

    // show things that should be visible, hide what should be hidden
    jQuery('.hide-if-no-js').show();
    jQuery('.hide-if-js').hide();

//-->
</script>
<?php
    }

    public function edit_node_info_form($hash) {
	$node = WordTrailsGlobal::getNode($hash);
	if (!is_object($node)) return false;
	$jumphash = "#node_info"; ?>

<a name="<?php echo substr($jumphash,1); ?>"></a>
<?php
if(strtolower($node->getType()) == "trail") {
    $trail = &$node;
    $unsaved = $trail->hasUnsaved();
?>
<div id="commit" style="text-align:right;"<?php echo $unsaved ? "" : " class=\"hide-if-js\""; ?> />
<form name="save_trail" style="display:inline;" method="post" action="<?php echo $this->href_to_submit($trail->getHash()); ?>">
    <input type="submit" class="<?php wt_button_class(); ?>" style="font-size:8pt;padding:1px;" value="Save & Publish" />
    <input type="hidden" name="process[save][func]" value="save_node" />
    <input type="hidden" name="process[save][args][hash]" value="<?php echo $trail->getHash(); ?>" />
</form>
<form name="revert_trail" style="display:inline;" method="post" action="<?php echo $this->href_to_submit($trail->getHash()); ?>">
    <input type="submit" class="<?php wt_button_class(); ?>" style="font-size:8pt;padding:1px;" value="<?php echo is_numeric($trail->getNodeID()) ? "Revert to Saved" : "Discard Changes"; ?>" />
    <input type="hidden" name="process[save][func]" value="revert_node" />
    <input type="hidden" name="process[save][args][hash]" value="<?php echo $trail->getHash(); ?>" />
</form>
</div><?php
}
?>
<div style="float:left" id="TrailViewer">
<form name="edit_node_info" method="post" action="<?php echo $this->href_to_submit($hash); ?>" style="display: inline;"><?php
	if ($node->delete_on_save) { ?>
<input type="hidden" name="process[info][func]" value="un_delete_node" />
<input type="hidden" name="process[info][args][hash]" value="<?php echo $node->getHash(); ?>" />
<?php $node->editInfoForm(); ?>
<input type="submit" class="<?php wt_button_class(); ?>" value="Restore" /><?php
	} else { ?>
<input type="hidden" name="process[info][func]" value="edit_node_info" />
<input type="hidden" name="process[info][args][hash]" value="<?php echo $node->getHash(); ?>" />
<?php $node->editInfoForm(); ?>
<input type="submit" class="<?php wt_button_class(); ?>" value="Update" />
</form><form name="delete_node" method="post" action="<?php echo $this->href_to_submit($hash); ?>" style="display: inline;">
<input type="hidden" name="process[info][func]" value="delete_node" />
<input type="hidden" name="process[info][args][hash]" value="<?php echo $node->getHash(); ?>" />
<input type="submit" class="<?php wt_button_class(); ?>" value="Delete" /><?php
	} ?>
</form>
</div><?php
	if (strtolower($node->getType()) == "trail") {
	    $sid = session_id();
	    $session = WordTrailsAnalytics::pullDetailsFor($node->getHash(), $sid);
	    $follows = null;
	    if (isset($session[$node->getHash()]) && isset($session[$node->getHash()][$sid]))
		$follows = $session[$node->getHash()][$sid]["hit"];
	    unset($session);
	    if (!empty($follows)) {
		    $follows = array_filter($follows, array("WordTrailsUtilities", "onlyWEV"));
		    if (!empty($follows)) {
			    global $wp_wt_sort;
			    $old_sort = $wp_wt_sort;
			    $wp_wt_sort = "Stamp";
			    uasort($follows, array("WordTrailsUtilities", "array_object_sort"));
			    $wp_wt_sort = $old_sort;
			    //$lastFollowed = end($follows);
			    //reset($follows);
			    $follows = array_map(array("WordTrailsUtilities", "extractNodeHash"), array_values($follows));
		    }
	    }
	    $xml = WordTrailsUtilities::redirect_link_to("xml", $node->getHash());
	    $flashvars = array();
	    if (!empty($follows)) $flashvars["follows"] = str_replace('"', "'", json_encode($follows));
	    $flashvars["save_uri"] = urlencode(WordTrailsUtilities::redirect_link_to("tv_save", 1));
	    $flashvars["redirect_after_save"] = urlencode($this->href_to_submit($hash));
	    $flashvars["node_target"] = "_blank";
	    $override_backup_div = true;
	    include "autosave_admin.inc";
	    include "visual_trail_wide.inc";
	}
	?>
<br class="clear" /><?php
    }

    public function edit_node_info($args) {
	extract($args);
	if (!isset($hash)) return false;
	$node = WordTrailsGlobal::getNode($hash);
	if (!is_object($node)) return false;
	$node->editInfoFormProcess($args);
    }
    public function edit_node_add_children_form($hash) {
	global $wpdb;
	$node = WordTrailsGlobal::getNode($hash);
	if (!is_object($node) || empty($node->trail_hash)) return false;
	$trail = WordTrailsGlobal::getNode($node->trail_hash);
	if (!is_object($trail) || empty($trail->child_hashes)) return false;
	$jumphash = "#add_children";
	$type = $node->getType();
	$child_type = "child";
	$children_type = "children";
	if ($type == "trail") {
	    $child_type = "marker";
	    $children_type = "markers";
	}
	list($trail_live, $trail_db) = $trail->getLiveChildrenAndHashes();
	$trail_live = array_diff($trail_live, array($hash), $node->child_hashes);
	$trail_db = array_diff($trail_db, array($hash), $node->child_hashes);

	$cpp = "siblings_per_page";
	$cp = "sibling_paged";
	$total = (count($trail_live)+count($trail_db));
	$maxcount = (int)$_GET[$cpp]>0 ? (int)$_GET[$cpp]:10;
	$page = (int)$_GET[$cp]>0 ? (int)$_GET[$cp]:1;
	$start = ($page-1)*$maxcount;
	while ($start > $total) {
	    $start -= $maxcount;
	    $page--;
	}
	$count = 0;
	$page_links = paginate_links( array(
	    'base' => add_query_arg( $cp, '%#%' ) . $jumphash,
	    'format' => '',
	    'total' => ceil($total/$maxcount),
	    'current' => $page
	));
	?>
<a name="<?php echo substr($jumphash, 1); ?>"></a>
<?php $this->num_per_page($cpp, 10, $cp, $jumphash); ?>
<form name="edit_node_add_children" id="children_add_existing" method="post" action="<?php echo $this->href_to_submit($hash) . "#edit_children"; ?>">
<input type="hidden" id="existing_children_func" name="process[add_children][func]" value="" />
<input type="hidden" name="process[add_children][args][parenthash]" value="<?php echo $hash; ?>" />
<div class="tablenav">
    <?php if ($page_links) echo "<div class='tablenav-pages'>{$page_links}</div>"; ?>
    <input type="submit" class="<?php wt_button_class(); ?>" onclick="jQuery('#existing_children_func').attr('value', 'add_existing_children');" value="Add Children" />
</div>
<br class="clear" />
    <table class="widefat" cellspacing="4">
    <thead>
    <tr valign="top" class="thead">
	<th scope="col" class="check-column"><input type="checkbox"<?php wt_checkAll("document.getElementById('children_add_existing')");?> /></th>
        <th scope="col"><?php _e( 'Marker Name', WordTrailsGlobal::DOMAIN ); ?></th>
        <th scope="col"><?php _e( 'Caption', WordTrailsGlobal::DOMAIN ); ?></th>
        <th scope="col" class="num"><?php _e( 'Number of Children', WordTrailsGlobal::DOMAIN ); ?></th>
        <th scope="col" class="num"><?php _e( 'Unsaved Changes', WordTrailsGlobal::DOMAIN ); ?></th>
        <th scope="col" width="60px"><?php _e( 'Action(s)' ); ?></th>
    </tr>
    </thead>
    <tbody>
    <?php
    if (!empty($trail_db) || !empty($trail_live)) {
	$alternate = true;
	foreach ($trail_live as $hash) {
	    $alternate = !$alternate;
	    $child = WordTrailsGlobal::getNode($hash);
	    if (!is_object($child)) continue;
	    $count++;
	    if ($count <= $start || $count > $start+$maxcount) continue; ?>

    <tr class="<?php echo in_array($hash, $this->flash_change) ?"flash-change":""; echo $alternate ? " alternate":""; ?>">
	<th scope="row" class="check-column"><input type="checkbox" name="process[add_children][args][children][]" value="<?php echo $hash; ?>" /></th>
	<td><?php echo $child->getName(); ?></td>
	<td><?php echo $child->getShortDesc(); ?></td>
	<td class="num"><?php echo count($child->child_hashes); ?></td>
	<td class="num"><?php echo count($child->unsaved()); ?></td>
	<td>
	    <input id="form_modify_func_<?php echo $child->getNodeID(); ?>" type="hidden" name="process[modify_<?php echo $child->getNodeID(); ?>][func]" value="" />
	    <input id="form_modify_arg_hash_<?php echo $child->getNodeID(); ?>" type="hidden" name="process[modify_<?php echo $child->getNodeID(); ?>][args][hash]" value="<?php echo $child->getHash(); ?>" />
	    <a href="<?php echo $this->href_to_edit_node($child->getHash()); ?>" class="ltoh"><img src="<?php echo WordTrailsGlobal::$urlpath; ?>imgs/buttons/edit_l.png" alt="Edit" /></a><?php
	    if ($child->delete_on_save) { ?>

	    <a href="#" class="ltoh" onclick="jQuery('#form_modify_func_<?php echo $child->getNodeID(); ?>').attr('value','un_delete_node');jQuery(this).parents('form').submit();"><img src="<?php echo WordTrailsGlobal::$urlpath; ?>imgs/buttons/undelete_l.png" alt="Un-Delete" /></a><?php
	    } else if ($type == "trail") { ?>

	    <a href="#" class="ltoh" onclick="jQuery('#form_modify_func_<?php echo $child->getNodeID(); ?>').attr('value','delete_node');jQuery(this).parents('form').submit();"><img src="<?php echo WordTrailsGlobal::$urlpath; ?>imgs/buttons/delete_l.png" alt="Delete" /></a><?php
	    } ?>

	</td>
    </tr><?php
	}
	if (!empty($trail_db)) {
	    $sql = "SELECT n.NodeID, n.Hash, n.Name, n.ShortDesc FROM " . WordTrailsGlobal::$tables->node . " as n WHERE n.Hash IN (" .
		implode(",",array_map(array("WordTrailsUtilities", "wrap_with_quotes"), $trail_db)) .
		") ORDER BY n.NodeID DESC";
	    $children_from_db = $wpdb->get_results($sql, OBJECT);
	    $countsql = "SELECT COUNT(nr.ChildID) FROM " . WordTrailsGlobal::$tables->node_rel . " as nr WHERE nr.ParentID = %d GROUP BY nr.ParentID";
	    if (is_array($children_from_db)) {
		foreach ($children_from_db as $child) {
		    $count++;
		    $alternate = !$alternate;
		    if ($count <= $start || $count > $start+$maxcount) continue;
		    $numchildren = $wpdb->get_var($wpdb->prepare($countsql, $child->NodeID));
		    $numchildren = (int)$numchildren;?>
    <tr class="<?php echo in_array($child->Hash, $this->flash_change) ?"flash-change":""; echo $alternate ? " alternate":""; ?>">
	<th scope="row" class="check-column"><input type="checkbox" name="process[add_children][args][children][]" value="<?php echo $child->Hash; ?>" /></th>
	<td><?php echo $child->Name; ?></td>
	<td><?php echo $child->ShortDesc; ?></td>
	<td class="num"><?php echo $numchildren; ?></td>
	<td class="num">0</td>
	<td>
	    <input id="form_modify_func_<?php echo $child->NodeID; ?>" type="hidden" name="process[modify_<?php echo $child->NodeID; ?>][func]" value="" />
	    <input id="form_modify_arg_hash_<?php echo $child->NodeID; ?>" type="hidden" name="process[modify_<?php echo $child->NodeID; ?>][args][hash]" value="<?php echo $child->Hash; ?>" />
	    <a href="<?php echo $this->href_to_edit_node($child->Hash); ?>" class="ltoh"><img src="<?php echo WordTrailsGlobal::$urlpath; ?>imgs/buttons/edit_l.png" alt="Edit" /></a><?php
	     if ($type == "trail") { ?>

	    <a href="#" class="ltoh" onclick="jQuery('#form_modify_func_<?php echo $child->NodeID; ?>').attr('value','delete_node');jQuery(this).parents('form').submit();"><img src="<?php echo WordTrailsGlobal::$urlpath; ?>imgs/buttons/delete_l.png" alt="Delete" /></a><?php
	     } ?>

	</td>
    </tr><?php
		}
	    }
	}
    } else { ?>

    <tr><td colspan="6" align="center"><i>No Other Markers Found</i></td></tr><?php
    }

    ?>
    </tbody>
</table>
<div class="tablenav">
    <?php if ($page_links) echo "<div class='tablenav-pages'>{$page_links}</div>"; ?>
    <input type="submit" class="<?php wt_button_class(); ?>" onclick="jQuery('#existing_children_func').attr('value', 'add_existing_children');" value="Add Children" />
</div>
<br class="clear" />
</form><?php
    }

    public function add_existing_children($args) {
	extract($args);
	if (!is_array($children) || empty($children)) return false;
	$parent = WordTrailsGlobal::getNode($parenthash);
	if (!is_object($parent)) return false;
	$parent->addChild($children);
	$this->addMessage("Added " . count($children) . " Child". (count($children) != 1 ? "ren" : "") . " to this node", "added_children_notice");
	$this->flash_change += $children;
	/*
	?><div id="message" class="updated fade">Added <?php echo count($children); ?> Child<?php echo count($children) != 1 ? "ren" : "";?> to this node</div><?php
	*/
    }

    public function delete_relationships($args) {
	extract($args);
	if (!is_array($children) || empty($children)) return false;
	$parent = WordTrailsGlobal::getNode($parenthash);
	if (!is_object($parent)) return false;
	$parent->removeChild($children);
	$this->addMessage("Removed " . count($children) . " Child" . (count($children) != 1 ? "ren" : "") . " from this node", "removed_childre_notice");
	$this->flash_change += $children;
	/*
	?><div id="message" class="updated fade">Removed <?php echo count($children); ?> Child<?php echo count($children) != 1 ? "ren" : "";?> from this node</div><?php
	*/
    }

    public function num_per_page($var, $default_npp = 10, $vars_to_trash = null, $jumphash = "", $class = "") {
	?><div class="alignright" style="font-size: 12px;"><ul class="subsubsub" style="display:inline;"><?php
	$size_links = array();
	if (!isset( $_GET[$var]) || 0 == @(int)$_GET[$var]) $_GET[$var] = $default_npp;
	foreach ($this->option_num_per_page as $size) {
	    if ((int)$_GET[$var] == $size)
		$size_links[] = "<li><a href='javascript:void(0)' class='current'>{$size}</a>";
	    else
	        $size_links[] = "<li><a href='{$this->build_href(array("$var" => $size), $vars_to_trash)}{$jumphash}' {$class}>{$size}</a>";
	}
	echo implode(' |</li>', $size_links) . '</li>';
	unset($size_links);
	if ($_GET[$var] == $default_npp)
	    unset($_GET[$var]);
	?></ul></div><br class="clear" /><?php
    }

    public function edit_node_children_form($hash) {
	global $wpdb;
	$node = WordTrailsGlobal::getNode($hash);
	if (!is_object($node)) return false;
	$jumphash = "#edit_children";
	$type = $node->getType();
	$child_type = "child";
	$children_type = "children";
	if ($type == "trail") {
	    $child_type = "marker";
	    $children_type = "markers";
	}
	$cpp = "children_per_page";
	$cp = "child_paged";
	$total = count($node->child_hashes);
	$maxcount = (int)$_GET[$cpp]>0 ? (int)$_GET[$cpp]:10;
	$page = (int)$_GET[$cp]>0 ? (int)$_GET[$cp]:1;
	$start = ($page-1)*$maxcount;
	while ($start > $total) {
	    $start -= $maxcount;
	    $page--;
	}
	$count = 0;
	$page_links = paginate_links( array(
	    'base' => add_query_arg( $cp, '%#%' ) . $jumphash,
	    'format' => '',
	    'total' => ceil($total/$maxcount),
	    'current' => $page
	));
	?>
<a name="<?php echo substr($jumphash,1); ?>"></a>
<?php $this->num_per_page($cpp, 10, $cp, $jumphash); ?>
<form name="edit_node_del_children" id="children_del_rel" method="post" action="<?php echo $this->href_to_submit($hash) . $jumphash; ?>">
<input type="hidden" id="remove_children_func" name="process[del_rel][func]" value="" />
<input type="hidden" name="process[del_rel][args][parenthash]" value="<?php echo $hash; ?>" />
<?php if ($page_links || $type != "trail") { ?><div class="tablenav">
    <?php if ($page_links) echo "<div class='tablenav-pages'>{$page_links}</div>"; ?>
    <?php if ($type != "trail") { ?><input type="submit" class="<?php wt_button_class(); ?>" onclick="jQuery('#remove_children_func').attr('value', 'delete_relationships');" value="Delete Relationships" /><?php } ?>

</div><?php } ?>

<br class="clear" />
    <table class="widefat" cellspacing="4">
    <thead>
    <tr>
	<th scope="col" class="check-column"><input type="checkbox" <?php if ($type == "trail") { ?>disabled="disabled"<?php } else { wt_checkAll("document.getElementById('children_del_rel')"); } ?> /></th>
        <th scope="col"><?php _e( ucwords($child_type) . ' Name', WordTrailsGlobal::DOMAIN ); ?></th>
        <th scope="col"><?php _e( 'Caption', WordTrailsGlobal::DOMAIN ); ?></th>
        <th scope="col" class="num"><?php _e( 'Number of Children', WordTrailsGlobal::DOMAIN ); ?></th>
        <th scope="col" class="num"><?php _e( 'Default', WordTrailsGlobal::DOMAIN ); ?></th>
        <th scope="col" class="num"><?php _e( 'Unsaved Changes', WordTrailsGlobal::DOMAIN ); ?></th>
        <th scope="col" width="90px"><?php _e( 'Action(s)' ); ?></th>
    </tr>
    </thead>
    <tbody>
    <?php
    if ($node->hasChildren()) {
	list($live_children, $hashes_from_db) = $node->getLiveChildrenAndHashes();
	$alternate = true;
	foreach ($live_children as $hash) {
	    $alternate = !$alternate;
	    $child = WordTrailsGlobal::getNode($hash);
	    if (!is_object($child)) continue;
	    $count++;
	    if ($count <= $start) continue;
	    if ($count > $start+$maxcount) break; ?>

    <tr class="<?php echo in_array($hash, $this->flash_change) ?"flash-change":""; echo $alternate ? " alternate":""; ?>">
	<th scope="row" class="check-column"><input type="checkbox" name="process[del_rel][args][children][]" value="<?php echo $child->getHash(); ?>" <?php echo ($type == "trail" && $child->getType() != "trail") ? "disabled=\"disabled\" " : ""; ?>/></th>
	<td><?php echo $child->getName(); ?></td>
	<td><?php echo $child->getShortDesc(); ?></td>
	<td class="num"><?php echo count($child->child_hashes); ?></td>
	<td class="num"><input type="radio" name="child_radio_<?php echo $hash; ?>" onclick="setDefault('<?php echo $node->getHash(); ?>','<?php echo $hash; ?>', '<?php echo $jumphash; ?>');return false;" <?php echo ($node->isDefault($hash) ? "checked=\"checked\"" : ""); ?> /></td>
	<td class="num"><?php echo count($child->unsaved()); ?></td>
	<td>
	    <input id="form_modify_func_<?php echo $child->getNodeID(); ?>" type="hidden" name="process[modify_<?php echo $child->getNodeID(); ?>][func]" value="" />
	    <input id="form_modify_arg_hash_<?php echo $child->getNodeID(); ?>" type="hidden" name="process[modify_<?php echo $child->getNodeID(); ?>][args][hash]" value="<?php echo $child->getHash(); ?>" />
	    <a href="<?php echo $this->href_to_edit_node($child->getHash()); ?>" class="ltoh"><img src="<?php echo WordTrailsGlobal::$urlpath; ?>imgs/buttons/edit_l.png" alt="Edit" /></a><?php
	    if ($child->delete_on_save) { ?>

	    <a href="#" class="ltoh" onclick="jQuery('#form_modify_func_<?php echo $child->getNodeID(); ?>').attr('value','un_delete_node');jQuery(this).parents('form').submit();"><img src="<?php echo WordTrailsGlobal::$urlpath; ?>imgs/buttons/undelete_l.png" alt="Un-Delete"/></a><?php
	    } else if ($type == "trail") { ?>

	    <a href="#" class="ltoh" onclick="jQuery('#form_modify_func_<?php echo $child->getNodeID(); ?>').attr('value','delete_node');jQuery(this).parents('form').submit();"><img src="<?php echo WordTrailsGlobal::$urlpath; ?>imgs/buttons/delete_l.png" alt="Delete"/></a><?php
	    } ?>

	</td>
    </tr><?php
	}
	if (!empty($hashes_from_db)) {
	    $sql = "SELECT n.NodeID, n.Hash, n.Name, n.ShortDesc, n.Type FROM " . WordTrailsGlobal::$tables->node . " as n ";
	    $sql .= "WHERE n.Hash IN (" .
		implode(",",array_map(array("WordTrailsUtilities", "wrap_with_quotes"), $hashes_from_db)) .
		") ORDER BY n.NodeID DESC";
	    $children_from_db = $wpdb->get_results($sql, OBJECT);
	    $countsql = "SELECT COUNT(nr.ChildID) FROM " . WordTrailsGlobal::$tables->node_rel . " as nr WHERE nr.ParentID = %d GROUP BY nr.ParentID";
	    if (is_array($children_from_db)) {
		foreach ($children_from_db as $child) {
		    $alternate = !$alternate;
		    $count++;
		    if ($count <= $start || $count > $start+$maxcount) continue;
		    $numchildren = $wpdb->get_var($wpdb->prepare($countsql, $child->NodeID));
		    $numchildren = (int)$numchildren;?>
    <tr class="<?php echo in_array($child->Hash, $this->flash_change) ?"flash-change":""; echo $alternate ? " alternate":""; ?>">
	<th scope="row" class="check-column"><input type="checkbox" name="process[del_rel][args][children][]" value="<?php echo $child->Hash; ?>" <?php echo ($type == "trail" && $child->Type != "trail") ? "disabled=\"disabled\" " : ""; ?>/></th>
	<td><?php echo $child->Name; ?></td>
	<td><?php echo $child->ShortDesc; ?></td>
	<td class="num"><?php echo $numchildren; ?></td>
	<td class="num"><input type="radio" name="child_radio_<?php echo $child->Hash; ?>" onclick="setDefault('<?php echo $node->getHash(); ?>','<?php echo $child->Hash; ?>', '<?php echo $jumphash; ?>');return false;" <?php echo ($node->isDefault($child->Hash) ? "checked=\"checked\"" : ""); ?> /></td>
	<td class="num">0</td>
	<td>
	    <input id="form_modify_func_<?php echo $child->NodeID; ?>" type="hidden" name="process[modify_<?php echo $child->NodeID; ?>][func]" value="" />
	    <input id="form_modify_arg_hash_<?php echo $child->NodeID; ?>" type="hidden" name="process[modify_<?php echo $child->NodeID; ?>][args][hash]" value="<?php echo $child->Hash; ?>" />
	    <a href="<?php echo $this->href_to_edit_node($child->Hash); ?>" class="ltoh"><img src="<?php echo WordTrailsGlobal::$urlpath; ?>imgs/buttons/edit_l.png" alt="Edit" /></a><?php
	     if ($type == "trail") { ?>

	    <a href="#" class="ltoh" onclick="jQuery('#form_modify_func_<?php echo $child->NodeID; ?>').attr('value','delete_node');jQuery(this).parents('form').submit();"><img src="<?php echo WordTrailsGlobal::$urlpath; ?>imgs/buttons/delete_l.png" alt="Delete" /></a><?php
	     } ?>

	</td>
    </tr><?php
		}
	    }
	}
    } else { ?>

    <tr><td colspan="7" align="center"><i>No <?php echo ucwords($children_type); ?> Found</i></td></tr><?php
    }

    ?>
    </tbody>
</table>
<div class="tablenav">
    <?php if ($page_links) echo "<div class='tablenav-pages'>{$page_links}</div>"; ?>
    <?php if ($type != "trail") { ?><input type="submit" class="<?php wt_button_class(); ?>" onclick="jQuery('#remove_children_func').attr('value', 'delete_relationships');" value="Delete Relationships" /><?php } ?>

</div>
<br class="clear" />
</form><?php
    }

    public function edit_node_add_post_form($hash) {
	global $wpdb, $wp_locale, $wp_query;
	$node = WordTrailsGlobal::getNode($hash);
	$type = $node->getType();
	$jumphash = "#add_post";
	$child_type = "child";
	$children_type = "children";
	if ($type == "trail") {
	    $child_type = "marker";
	    $children_type = "markers";
	}
	$default_ppp = 10;
	$ppp = "posts_per_page";
	if ( !isset( $_GET['paged'] ) )
	    $_GET['paged'] = 1;
	if ( !isset($_GET[$ppp]))
	    $_GET[$ppp] = $default_ppp;
	$vars_to_keep = array($ppp, "s", "cat", "tag", "m", "author", "post_status");
	list($post_stati, $avail_post_stati) = wp_edit_posts_query();
	set_query_var("posts_per_page", $_GET[$ppp]);
	$wp_query->query($wp_query->query_vars);?>

<a name="<?php echo substr($jumphash,1); ?>"></a>
<form id="posts-filter" action="<?php echo $jumphash; ?>" method="get"><?php
foreach (array("page", "mode", "hash") as $var) { ?>

    <input type="hidden" name="<?php echo $var; ?>" value="<?php echo attribute_escape($_GET[$var]); ?>" /><?php
}
?>

<h2 style="float:left;margin:0px;padding:0px;border-bottom: none;"><?php
	$post_status_label = __('All Posts');
	if ( isset($_GET['post_status']) && in_array( $_GET['post_status'], array_keys($post_stati) ) )
        $post_status_label = $post_stati[$_GET['post_status']][1];
	if ( $post_listing_pageable && !is_archive() && !is_search() )
		$h2_noun = is_paged() ? sprintf(__( 'Previous %s' ), $post_status_label) : sprintf(__('Latest %s'), $post_status_label);
	else
		$h2_noun = $post_status_label;
	// Use $_GET instead of is_ since they can override each other
	$h2_author = '';
	$_GET['author'] = (int) $_GET['author'];
	if ( $_GET['author'] != 0 ) {
		if ( $_GET['author'] == '-' . $user_ID ) { // author exclusion
			$h2_author = ' ' . __('by other authors');
		} else {
			$author_user = get_userdata( get_query_var( 'author' ) );
			$h2_author = ' ' . sprintf(__('by %s'), wp_specialchars( $author_user->display_name ));
		}
	} else {
	    unset($_GET['author']);
	}
	$h2_search = isset($_GET['s'])   && $_GET['s']   ? ' ' . sprintf(__('matching &#8220;%s&#8221;'), wp_specialchars( get_search_query() ) ) : '';
	$h2_cat    = isset($_GET['cat']) && $_GET['cat'] ? ' ' . sprintf( __('in &#8220;%s&#8221;'), single_cat_title('', false) ) : '';
	$h2_tag    = isset($_GET['tag']) && $_GET['tag'] ? ' ' . sprintf( __('tagged with &#8220;%s&#8221;'), single_tag_title('', false) ) : '';
	$h2_month  = isset($_GET['m'])   && $_GET['m']   ? ' ' . sprintf( __('during %s'), single_month_title(' ', false) ) : '';
	printf( _c( '%1$s%2$s%3$s%4$s%5$s%6$s|You can reorder these: 1: Posts, 2: by {s}, 3: matching {s}, 4: in {s}, 5: tagged with {s}, 6: during {s}' ), $h2_noun, $h2_author, $h2_search, $h2_cat, $h2_tag, $h2_month );
	?>
</h2>
	<div style="float:right;">
	       <input type="text" id="post-search-input" name="s" value="<?php the_search_query(); ?>" />
	       <input type="submit" value="<?php _e( 'Search Posts' ); ?>" class="<?php wt_alt_button_class(); ?>" />
	</div>
	<br style="clear:both;" />
	<div style="float:left;"><ul class="subsubsub" style="margin:0px;padding:0px;">
	<?php
	$status_links = array();
	$num_posts = wp_count_posts( 'post', 'readable' );
	$class = empty( $_GET['post_status'] ) ? ' class="current"' : '';
	$status_links[] = "<li><a href='" . $this->href_to_edit_node($hash, array_diff($vars_to_keep, array("post_status"))) . "{$jumphash}' $class>" . __('All Posts') . '</a>';
	foreach ( $post_stati as $status => $label ) {
		$class = '';

		if ( !in_array( $status, $avail_post_stati ) )
			continue;

		if ( empty( $num_posts->$status ) )
			continue;
		if ( $status == $_GET['post_status'] )
			$class = ' class="current"';

		$status_links[] = "<li><a href='" . $this->href_to_edit_node($hash, array_diff($vars_to_keep, array("post_status"))) . "&amp;post_status={$status}{$jumphash}' $class>" .
		sprintf( __ngettext( $label[2][0], $label[2][1], $num_posts->$status ), number_format_i18n( $num_posts->$status ) ) . '</a>';
	}
	echo implode( ' |</li>', $status_links ) . '</li>';
	unset( $status_links );
	?>
	</ul></div>
<?php $this->num_per_page($ppp, $default_ppp, $pp, $jumphash, $class); ?>

<?php if ( isset($_GET['post_status'] ) ) : ?>
<input type="hidden" name="post_status" value="<?php echo attribute_escape($_GET['post_status']) ?>" />
<?php endif; ?>
<?php if ( isset($_GET['posts_per_page'] ) ) : ?>
<input type="hidden" name="posts_per_page" value="<?php echo attribute_escape($_GET['posts_per_page']) ?>" />
<?php endif; ?>
<div class="tablenav">

<?php
$page_links = paginate_links( array(
	'base' => add_query_arg( 'paged', '%#%' ) . $jumphash,
	'format' => '',
	'total' => $wp_query->max_num_pages,
	'current' => $_GET['paged']
));

if ( $page_links )
	echo "<div class='tablenav-pages'>$page_links</div>";
?>
<div class="alignleft">
<input type="button" class="<?php wt_button_class(); ?>" name="add_posts" value="Add Posts as <?php echo ucwords($children_type); ?>" onclick="jQuery('#add_posts_form').submit()" />
<?php
$arc_query = "SELECT DISTINCT YEAR(post_date) AS yyear, MONTH(post_date) AS mmonth FROM $wpdb->posts WHERE post_type = 'post' ORDER BY post_date DESC";

$arc_result = $wpdb->get_results( $arc_query );

$month_count = count($arc_result);

if ( $month_count && !( 1 == $month_count && 0 == $arc_result[0]->mmonth ) ) { ?>
<select name='m'>
<option<?php selected( @$_GET['m'], 0 ); ?> value='0'><?php _e('Show all dates'); ?></option>
<?php
foreach ($arc_result as $arc_row) {
	if ( $arc_row->yyear == 0 )
		continue;
	$arc_row->mmonth = zeroise( $arc_row->mmonth, 2 );

	if ( $arc_row->yyear . $arc_row->mmonth == $_GET['m'] )
		$default = ' selected="selected"';
	else
		$default = '';

	echo "<option$default value='$arc_row->yyear$arc_row->mmonth'>";
	echo $wp_locale->get_month($arc_row->mmonth) . " $arc_row->yyear";
	echo "</option>\n";
}
?>
</select>
<?php } ?>

<?php
$dropdown_options = array('show_option_all' => __('View all categories'), 'hide_empty' => 0, 'hierarchical' => 1,
	'show_count' => 0, 'orderby' => 'name', 'selected' => $_GET['cat']);
wp_dropdown_categories($dropdown_options);
do_action('restrict_manage_posts');
?>
<input type="submit" id="post-query-submit" value="<?php _e('Filter'); ?>" class="<?php wt_alt_button_class(); ?>" />
</div>
</div>
</form>

<br class="clear" />
<?php
//-----------Rows Start
?>
<form id="add_posts_form" name="edit_node_add_posts_as_children" method="post" action="<?php echo $this->href_to_submit($hash) . "#edit_children"; ?>">
<input type="hidden" name="process[addpost][func]" value="create_nodes_from_posts" />
<input type="hidden" name="process[addpost][args][parenthash]" value="<?php echo $hash; ?>" />
<table class="widefat">
	<thead>
	<tr>

<?php
$posts_columns = array_diff_key(wp_manage_posts_columns(), array('comments'=>1));
$posts_columns["cb"]= "<input type=\"checkbox\"" . wt_checkAll("document.getElementById('add_posts_form')", true) . " />";

foreach($posts_columns as $post_column_key => $column_display_name) {
	if ( 'cb' === $post_column_key )
		$class = ' class="check-column"';
	elseif ( 'comments' === $post_column_key )
		$class = ' class="num"';
	else
		$class = '';
?>
	<th scope="col"<?php echo $class; ?>><?php echo $column_display_name; ?></th>
<?php } ?>

	</tr>
	</thead>
	<tbody>
<?php
if ( have_posts() ) {
$bgcolor = '';
add_filter('the_title','wp_specialchars');

// Create array of post IDs.
$post_ids = array();
foreach ( $wp_query->posts as $a_post )
	$post_ids[] = $a_post->ID;

$comment_pending_count = get_pending_comments_num($post_ids);
global $post, $id;
while (have_posts()) : the_post();
$class = 'alternate' == $class ? '' : 'alternate';
global $current_user;
$post_owner = ( $current_user->ID == $post->post_author ? 'self' : 'other' );
$title = get_the_title();
if ( empty($title) )
	$title = __('(no title)');
?>
	<tr id='post-<?php echo $id; ?>' class='<?php echo trim( $class . ' author-' . $post_owner . ' status-' . $post->post_status ); ?>' valign="top">

<?php

foreach($posts_columns as $column_name=>$column_display_name) {

	switch($column_name) {

	case 'cb':
		?>
		<th scope="row" class="check-column"><?php if ( current_user_can( 'edit_post', $post->ID ) ) { ?><input type="checkbox" name="process[addpost][args][postids][]" value="<?php the_ID(); ?>" /><?php } ?></th>
		<?php
		break;
	case 'modified':
	case 'date':
		if ( '0000-00-00 00:00:00' == $post->post_date && 'date' == $column_name ) {
			$t_time = $h_time = __('Unpublished');
		} else {
			if ( 'modified' == $column_name ) {
				$t_time = get_the_modified_time(__('Y/m/d g:i:s A'));
				$m_time = $post->post_modified;
				$time = get_post_modified_time('G', true);
			} else {
				$t_time = get_the_time(__('Y/m/d g:i:s A'));
				$m_time = $post->post_date;
				$time = get_post_time('G', true);
			}
			if ( ( abs(time() - $time) ) < 86400 ) {
				if ( ( 'future' == $post->post_status) )
					$h_time = sprintf( __('%s from now'), human_time_diff( $time ) );
				else
					$h_time = sprintf( __('%s ago'), human_time_diff( $time ) );
			} else {
				$h_time = mysql2date(__('Y/m/d'), $m_time);
			}
		}
		?>
		<td><abbr title="<?php echo $t_time ?>"><?php echo apply_filters('post_date_column_time', $h_time, $post, $column_name) ?></abbr></td>
		<?php
		break;
	case 'title':
		?>
		<td><strong><!--a class="row-title" href="<?php echo get_permalink($post->ID); ?>" title="<?php echo attribute_escape(sprintf(__('View "%s"'), $title)); ?>"--><?php echo $title ?></strong>
		<?php if ( !empty($post->post_password) ) { _e(' &#8212; <strong>Protected</strong>'); } elseif ('private' == $post->post_status) { _e(' &#8212; <strong>Private</strong>'); } ?></td>
		<?php
		break;

	case 'categories':
		?>
		<td><?php
		$categories = get_the_category();
		if ( !empty( $categories ) ) {
			$out = array();
			foreach ( $categories as $c )
				$out[] = "<a href='" . $this->href_to_edit_node($hash, array_diff($vars_to_keep, array("tag", "cat"))) . "&amp;cat={$c->term_id}{$jumphash}'> " . wp_specialchars(sanitize_term_field('name', $c->name, $c->term_id, 'category', 'display')) . "</a>";
			echo join( ', ', $out );
		} else {
			_e('Uncategorized');
		}
		?></td>
		<?php
		break;

	case 'tags':
		?>
		<td><?php
		$tags = get_the_tags();
		if ( !empty( $tags ) ) {
			$out = array();
			foreach ( $tags as $c )
				$out[] = "<a href='" . $this->href_to_edit_node($hash, array_diff($vars_to_keep, array("tag", "cat"))) . "&amp;tag={$c->slug}{$jumphash}'> " . wp_specialchars(sanitize_term_field('name', $c->name, $c->term_id, 'post_tag', 'display')) . "</a>";
			echo join( ', ', $out );
		} else {
			_e('No Tags');
		}
		?></td>
		<?php
		break;

	case 'comments':
		?>
		<td class="num"><div class="post-com-count-wrapper">
		<?php
		$left = isset($comment_pending_count) ? $comment_pending_count[$post->ID] : 0;
		$pending_phrase = sprintf( __('%s pending'), number_format( $left ) );
		if ( $left )
			echo '<strong>';
		comments_number("<a href='edit.php?p=$id' title='$pending_phrase' class='post-com-count'><span class='comment-count'>" . __('0') . '</span></a>', "<a href='edit.php?p=$id' title='$pending_phrase' class='post-com-count'><span class='comment-count'>" . __('1') . '</span></a>', "<a href='edit.php?p=$id' title='$pending_phrase' class='post-com-count'><span class='comment-count'>" . __('%') . '</span></a>');
		if ( $left )
			echo '</strong>';
		?>
		</div></td>
		<?php
		break;

	case 'author':
		?>
		<td><a href="<?php echo $this->href_to_edit_node($hash, array_diff($vars_to_keep, array("author"))) . "&amp;author=";the_author_ID();echo $jumphash; ?>"><?php the_author() ?></a></td>
		<?php
		break;

	case 'status':
		?>
		<td>
		<a href="<?php the_permalink(); ?>" title="<?php echo attribute_escape(sprintf(__('View "%s"'), $title)); ?>" rel="permalink">
		<?php
		switch ( $post->post_status ) {
			case 'publish' :
			case 'private' :
				_e('Published');
				break;
			case 'future' :
				_e('Scheduled');
				break;
			case 'pending' :
				_e('Pending Review');
				break;
			case 'draft' :
				_e('Unpublished');
				break;
		}
		?>
		</a>
		</td>
		<?php
		break;

	case 'control_view':
		?>
		<td><a href="<?php the_permalink(); ?>" rel="permalink" class="view"><?php _e('View'); ?></a></td>
		<?php
		break;

	case 'control_edit':
		?>
		<td><?php if ( current_user_can('edit_post',$post->ID) ) { echo "<a href='post.php?action=edit&amp;post=$id' class='edit'>" . __('Edit') . "</a>"; } ?></td>
		<?php
		break;

	case 'control_delete':
		?>
		<td><?php if ( current_user_can('delete_post',$post->ID) ) { echo "<a href='" . wp_nonce_url("post.php?action=delete&amp;post=$id", 'delete-post_' . $post->ID) . "' class='delete'>" . __('Delete') . "</a>"; } ?></td>
		<?php
		break;

	default:
		?>
		<td><?php do_action('manage_posts_custom_column', $column_name, $id); ?></td>
		<?php
		break;
	}
}
?>
	</tr>
<?php
endwhile;
} else {
?>
  <tr style='background-color: <?php echo $bgcolor; ?>'>
    <td colspan="8"><?php _e('No posts found.') ?></td>
  </tr>
<?php
} // end if ( have_posts() )
?>
	</tbody>
</table>
</form>
<?php
//----------Rows End
?>
<div id="ajax-response"></div>

<div class="tablenav">
    <div class="alignleft">
	<input type="button" class="<?php wt_button_class(); ?>" name="add_posts" value="Add Posts as <?php echo ucwords($children_type); ?>" onclick="jQuery('#add_posts_form').submit()" />
    </div>

<?php
if ( $page_links )
	echo "<div class='tablenav-pages'>$page_links</div>";
?>

<br class="clear" />
</div>

<br class="clear" />
	<?php
    }
    public function create_nodes_from_posts($args) {
	extract($args);
	if (!isset($parenthash) || !is_array($postids) || empty($postids)) return false;
	$postids = array_filter($postids, is_numeric);
	foreach ($postids as $pid) {
	    $pid = (int)$pid;
	    $this->new_int_post_node($parenthash, $pid);
	}
	$this->addMessage("Added " . count($postids) . " new Markers from Posts", "markers_to_posts_notice");

	/*
	?><div id="message" class="updated fade">Added <?php echo count($postids); ?> new Markers from Posts</div><?php
	*/
    }

    public function edit_node_add_url_form($hash) {
	$node = WordTrailsGlobal::getNode($hash);
	if (!is_object($node)) return false;
	$this->continueMenu = false;
	$type = $node->getType();
	$child_type = "child";
	$children_type = "children";
	if ($type == "trail") {
	    $child_type = "marker";
	    $children_type = "markers";
	}?>
<form name="edit_node_add_url" method="post" action="<?php echo $this->href_to_submit($hash) . "#edit_children"; ?>">
    <input type="hidden" name="process[url][func]" value="new_ext_url_node" />
    <input type="hidden" name="process[url][args][parenthash]" value="<?php echo $hash; ?>" />
    <label>URL: <input type="text"<?php echo wt_text_box_size(30); ?>name="process[url][args][url]" value="" /></label><br />
    <span class="subsubsub">(This will likely be a dead end on your trail)</span><br /><br />
    <input type="submit" value="Add <?php echo ucwords($child_type); ?>" class="<?php wt_button_class(); ?>" />
</form>
	<?php
    }
    public function edit_node_add_p2p_form($hash) {
	echo "doesn't exist yet";
    }

    public function delete_node($args) {
	extract($args);
	if (!isset($hash)) return false;
	$node = WordTrailsGlobal::getNode($hash);
	if (!is_object($node)) return false;
	if ($node->getType() == "trail") {
	    $node->delete(true);
	    $this->addMessage("1 Trail deleted", "trail_deleted_notice");
	    /*
	    if (!isset($confirm)) {
		array_push($this->messages, "<strong>Are you sure you want to delete this trail and all of its markers?</strong>
    <br /><br /><form name=\"confirm\" method=\"post\" action=\"" . str_replace( '%7E', '~', $_SERVER['REQUEST_URI']) . "\">
    <input type=\"hidden\" name=\"process[confirm][func]\" value=\"delete_node\" />
    <input type=\"hidden\" name=\"process[confirm][args][hash]\" value=\"$hash\" />
    <input type=\"submit\" class=\"". wt_button_class(true) . "\" name=\"process[confirm][args][confirm]\" value=\"Yes - Delete\" />
    <input type=\"submit\" class=\"". wt_button_class(true) . "\" name=\"process[confirm][args][confirm]\" value=\"No - Cancel\" />
    </form>");
		$this->continueMenu = false;
	    } if ((isset($confirm) && stristr($confirm, "yes") !== false)) {
		$node->delete(true);
	    }
	    */
	} else {
	    $node->delete();
	    $this->flash_change[] = $node->getHash();
	}
    }
    public function un_delete_node($args) {
	extract($args);
	if (!isset($hash)) return false;
	$node = WordTrailsGlobal::getNode($hash);
	if (!is_object($node)) return false;
	$node->unFlagForDeletion();
	$this->flash_change[] = $hash;
    }

    public function new_ext_url_node($args) {
	extract($args);
	if (!isset($parenthash) || !isset($url)
	    || empty($parenthash) || empty($url)
	    || !preg_match('/^(http|https|ftp):\/\/[a-z0-9]+([\-\.]{1}[a-z0-9]+)*\.[a-z]{2,6}((:[0-9]{1,5})?\/.*)?$/i' ,$url)) return false;
	$parent = WordTrailsGlobal::getNode($parenthash);
	if (is_object($parent)) {
	    $node = WordTrailsGlobal::createNewNode(false);
	    $node->setURL($url);
	    $node->deriveSite();
	    $node->deriveType();
	    $node->setName($url);
	    if ($parent->getType() == "trail") {
		$node->trail_hash = $parenthash;
	    } else {
		$node->trail_hash = $parent->trail_hash;
		$trail = WordTrailsGlobal::getNode($parent->trail_hash);
		if (is_object($trail))
		    $trail->addChild($node->getHash());
	    }
	    $parent->addChild($node->getHash());
	    $this->flash_change[] = $node->getHash();
	}
    }

    public function new_int_post_node($parenthash, $id) {
	$node = WordTrailsGlobal::createNewNode(true);
	$node->setPostID($id);
	$parent = WordTrailsGlobal::getNode($parenthash);
	if (is_object($parent)) {
	    //this needs revisiting
	    if ($parent->getType() == "trail") {
		$node->trail_hash = $parenthash;
	    } else {
		$node->trail_hash = $parent->trail_hash;
		$trail = WordTrailsGlobal::getNode($parent->trail_hash);
		if (is_object($trail))
		    $trail->addChild($node->getHash());
	    }
	    $parent->addChild($node->getHash());
	    $this->flash_change[] = $node->getHash();
	}
    }

    //---------------------------------------------------------

    public function settingsMenu() {
	//$this->continueMenu = true;
	global $wpdb;
	//$this->processPOST();
	if (!$this->continueMenu) return;
	add_meta_box("wt_settings_trail_page", "Trails Display Page", array($this, "trail_page_form"), "wtsettings");
	add_meta_box("wt_settings_trail_permissions", "Trails Permissions", array($this, "trail_permissions"), "wtsettings");
	//add_meta_box("wt_settings_enable_analytics", "Analytics", array($this, "enable_analytics_form"), "wtsettings");
	//add_meta_box("wt_trailMeme_registration", "Index your trails on the Trailmeme search engine (optional, but strongly recommended)", array($this, "trailMeme_registration_form"), "wtsettings");
	//add_meta_box("wt_trail_sync", "Trail Search Indexing", array($this, "trail_sync_form"), "wtsettings");
	add_meta_box("wt_other_options", "Other Options", array($this, "other_options_form"), "wtsettings");
	?>

<div class="wrap">
    <h2 style="border-bottom:none"><?php _e( 'Settings', WordTrailsGlobal::DOMAIN ); ?></h2>
<?php //$this->printMessages(); ?>
<div id="poststuff">

<?php
do_meta_boxes("wtsettings", 'advanced', false);
wp_nonce_field( 'closedpostboxes', 'closedpostboxesnonce', false );
wp_nonce_field( 'meta-box-order', 'meta-box-order-nonce', false );
?>

</div>
</div>
<script language="JavaScript" type="text/javascript">
<!--

    var ajax_url = "<?php bloginfo( 'wpurl' ); ?>/wp-admin/admin-ajax.php";

    <?php post_box_toggles("wtsettings"); ?>

    // close postboxes that should be closed
    jQuery('.if-js-closed').removeClass('if-js-closed').addClass('closed');

    // show things that should be visible, hide what should be hidden
    jQuery('.hide-if-no-js').show();
    jQuery('.hide-if-js').hide();

//-->
</script>
	<?php

    }
    
    public function other_options_form() { ?>

    <form name="other_options" action="<?php echo $this->href_to_submit(); ?>" method="post" />
    <input type="hidden" name="process[reset][func]" value="reset_options" />
    <input type="hidden" name="process[reset][args][force]" value="true" />
    <input type="submit" class="<?php wt_button_class(); ?>" value="Reset Warnings" />
    </form><?php
    }
    public function reset_options($args) {
	foreach (array(WordTrailsGlobal::WARN_TRAIL_PAGE, WordTrailsGlobal::WARN_SIDEBAR, WordTrailsGlobal::WARN_WIDGET, WordTrailsGlobal::WARN_TRAILMEME) as $option) {
	    WordTrailsGlobal::update_option($option, 1);
	}
    }
    
    public function trail_permissions() {
	global $wp_roles;
	if (empty($wp_roles->roles)) return;?>

	<form id="trail_permissions" method="post" action="<?php echo $this->href_to_submit(); ?>">
	    <input type="hidden" name="process[trail_permissions][func]" value="update_trail_permissions" />
	    <label for="process[trail_permissions][args][role]">This user role and above can save trails:</label>
	    <select name="process[trail_permissions][args][role]">
		<?php
		global $wp_roles;
	$editable_roles = $wp_roles->roles;
	$selected = WordTrailsGlobal::get_option(WordTrailsGlobal::MIN_ROLE_SAVE);
	foreach( $editable_roles as $role => $details ) {
	    $name = translate_user_role($details['name'] );
	    echo "\n\t<option value='" . wp_specialchars($role) . "'";
	    if ( $selected == $role )
		echo " selected='selected'";
	    echo ">$name</option>";
	}
		?>

	    </select>
	    <input type="submit" class="<?php wt_button_class(); ?>" value="Update" />
	</form><?php
    }
    
    public function update_trail_permissions($args) {
	extract($args);
	WordTrailsGlobal::update_option(WordTrailsGlobal::MIN_ROLE_SAVE, $role);
	WordTrailsUtilities::addCapabilities();
    }

    public function enable_analytics_form() { ?>
    <form id="enable_analytics_form" method="post" action="<?php echo $this->href_to_submit(); ?>">
	<input type="hidden" name="process[enable_analytics][func]" value="update_analytics_settings" />
	<input type="hidden" name="process[enable_analytics][args][ignore]" value="ignore" />
	<label><input type="checkbox" name="process[enable_analytics][args][collect]" <?php echo (WordTrailsGlobal::get_option(WordTrailsGlobal::ANALYTICS_COLLECT_OPTION) == true) ? "checked=\"checked\" " : ""; ?>/>
	    Allow WordTrails to collect annonymous trail following data</label>
	<br />
	<label><input type="checkbox" name="process[enable_analytics][args][transmit]" <?php echo (WordTrailsGlobal::get_option(WordTrailsGlobal::ANALYTICS_TRANSMIT_OPTION) == true) ? "checked=\"checked\" " : ""; echo is_null(WordTrailsGlobal::get_option(WordTrailsGlobal::TRAILMEME_KEY_OPTION)) ? " disabled=\"disabled\"" : ""; ?>/>
	    Allow automatic analytics reporting to TrailMeme</label>
	<br />
	<input type="submit" class="<?php wt_button_class(); ?>" value="Update" />
    </form><?php
    }

    public function disable_analytics() {
	WordTrailsGlobal::update_option(WordTrailsGlobal::ANALYTICS_COLLECT_OPTION, 0);
	WordTrailsGlobal::update_option(WordTrailsGlobal::ANALYTICS_TRANSMIT_OPTION, 0);
	WordTrailsUtilities::unScheduleAnalytics();

    }
    public function enable_analytics() {
	WordTrailsGlobal::update_option(WordTrailsGlobal::ANALYTICS_COLLECT_OPTION, 1);
	WordTrailsGlobal::update_option(WordTrailsGlobal::ANALYTICS_TRANSMIT_OPTION, 1);
	WordTrailsUtilities::unScheduleAnalytics();
	WordTrailsUtilities::scheduleAnalytics();
    }

    public function trailMemeMenu() {
	//$this->continueMenu = true;
	global $wpdb;
	//$this->processPOST();
	if (!$this->continueMenu) return;


	//add_meta_box("wt_trailMeme_registration", "Index your trails on the Trailmeme search engine (optional, but strongly recommended)", array($this, "trailMeme_registration_form"), "trailmeme");
	//add_meta_box("wt_trail_sync", "Trail Search Indexing", array($this, "trail_sync_form"), "trailmeme");
	?>

<div class="wrap">
    <h2 style="border-bottom:none; margin:0px;"><?php _e( 'TrailMeme Settings', WordTrailsGlobal::DOMAIN ); ?></h2>
<?php //$this->printMessages(); ?>
<div id="poststuff">

<?php
do_meta_boxes("trailmeme", 'advanced', false);
wp_nonce_field( 'closedpostboxes', 'closedpostboxesnonce', false );
wp_nonce_field( 'meta-box-order', 'meta-box-order-nonce', false );
?>

</div>
</div>
<script language="JavaScript" type="text/javascript">
<!--

    var ajax_url = "<?php bloginfo( 'wpurl' ); ?>/wp-admin/admin-ajax.php";

    <?php post_box_toggles("trailmeme"); ?>

    // close postboxes that should be closed
    jQuery('.if-js-closed').removeClass('if-js-closed').addClass('closed');

    // show things that should be visible, hide what should be hidden
    jQuery('.hide-if-no-js').show();
    jQuery('.hide-if-js').hide();

//-->
</script><?php
    }

    public function trailMeme_registration_form() {
	if (!WordTrailsGlobal::get_option(WordTrailsGlobal::TRAILMEME_KEY_OPTION)) {
	    ?>

	(Will also list your trail updates on the <a href="http://trailmeme.com" target="_blank">Trailmeme</a> homepage)
	<form name="TrailMemeRegistration" action="https://trailmeme.com/site_reg/login_first" method="post" target="_blank">
	    <input type="hidden" name="site[hash]" value="<?php echo WordTrailsGlobal::get_option(WordTrailsGlobal::SITE_HASH_OPTION); ?>" />
	    <input type="hidden" name="site[url]" value="<?php echo get_option("home"); ?>" />
	    <input type="hidden" name="site[name]" value="<?php echo get_option("blogname"); ?>" />
	    <input type="hidden" name="site[caption]" value="<?php echo get_option("blogdescription"); ?>" />
	    <input type="hidden" name="version" value="WordTrails  <?php echo WordTrailsGlobal::VERSION; ?>" />
	    <input type="submit" value="Click here to register" class="<?php wt_button_class(); ?>" />
	</form>
	<h2 style="border-bottom:none; margin-bottom:0px;">Enter your Trailmeme Site Key</h2>
	<form name="SaveTrailMemeKey" action="<?php echo $this->href_to_submit(); ?>" method="post">
	    <input type="hidden" name="process[site_key][func]" value="store_site_key" />
	    <label>Site Key: <input type="text"<?php echo wt_text_box_size(20); ?>name="process[site_key][args][key]" /></label>
	    <input type="submit" value="Store" class="<?php wt_button_class(); ?>" />
	</form><?php
	} else {
	    ?>

	<h2 style="border-bottom:none;margin:0px;">You Trailmeme Site Key is stored</h2>
	<form name="delete_site_key" id="delete_site_key" action="<?php echo $this->href_to_submit(); ?>" method="post">
	    <input type="hidden" name="process[delete_key][func]" value="delete_site_key" />
	    <input type="hidden" name="process[delete_key][args][confirm]" value="0" />
	</form>
	<a href="javascript:void(0);" onclick="jQuery('#delete_site_key').submit();">Click here to delete your stored site key</a><br />
	<strong>Warning: This cannot be undone!</strong><?php
	}
    }

    public function trail_sync_form() {
	global $wpdb;
	if (!WordTrailsGlobal::get_option(WordTrailsGlobal::TRAILMEME_KEY_OPTION)) {
	    echo "You must register with Trailmeme before you can index your trails.";
	} else {
	    ?>

	<form name="send_trails" id="send_trails" action="javascript:GatherAndPushTrails('send_trails');" method="get">
	    <strong>Note:</strong> Trail Search Indexing is still beta and may not function properly.<br />
	    <strong>Note:</strong> Trail Search Indexing only works on trails stored in the database. If you have any unsaved changes on any trails, they will be <strong>DISCARDED</strong> before upload.
	<div class="tablenav"><input type="submit" class="<?php wt_button_class(); ?>" value="Index Selected Trails" /></div><br class="clear" />
	<table class="widefat">
	    <thead>
		<tr>
		    <th scope="col" class="check-column" width="50"><input type="checkbox"<?php wt_checkAll("document.getElementById('send_trails')"); ?> /></th>
		    <th scope="col" width="300">Trail Name</th>
		    <th scope="col" class="num" width="200">Last Modified</th>
		    <th scope="col" class="num">Last Indexed</th>
		</tr>
	    </thead>
	    <tbody><?php
	    $sync_dates = WordTrailsGlobal::getTrailSyncDates();
	    $trails = WordTrailsGlobal::getAllTrailNodes("Name, Modified");
	    $alternate = false;
	    foreach ($trails["db"] as $trail) {
		$alternate = !$alternate; ?>

		<tr id="tr_<?php echo $trail->Hash; ?>"<?php echo $alternate ? " class=\"alternate\"" : ""; ?> >
		    <th scope="row" class="check-column"><input type="checkbox" name="<?php echo $trail->Hash; ?>" /></th>
		    <td><a href="<?php echo $this->href_to_edit_node($trail->Hash); ?>" title="View/Edit Trail"><?php echo $trail->Name; ?></a></td>
		    <td class="num"><?php echo WordTrailsUtilities::stampToAbbr($trail->Modified); ?></td>
		    <td class="num sync_date"><?php echo (isset($sync_dates[$trail->Hash]) ? WordTrailsUtilities::stampToAbbr($sync_dates[$trail->Hash]) : "<i>Never indexed</i>"); ?></td>
		</tr><?php
	    }
	    foreach ($trails["live"] as $trail) {
		if (!is_numeric($trail->getNodeID())) continue;
		$alternate = !$alternate; ?>

		<tr id="tr_<?php echo $trail->getHash();?>"<?php echo $alternate ? " class=\"alternate\"" : ""; ?>>
		    <th scope="row" class="check-column"><input type="checkbox" name="<?php echo $trail->getHash(); ?>" /></th>
		    <td><a href="<?php echo $this->href_to_edit_node($trail->getHash()); ?>" title="View/Edit Trail"><?php echo $trail->getName(); ?></a><?php
		    if ($trail->hasUnsaved()) {
			echo " - " . count($trail->unsaved()) . " Unsaved Change(s) will be lost";
		    } ?>

		    <td class="num"><?php echo WordTrailsUtilities::stampToAbbr($trail->getModified()); ?></td>
		    <td class="num sync_date"><?php echo (isset($sync_dates[$trail->getHash()]) ? WordTrailsUtilities::stampToAbbr($sync_dates[$trail->getHash()]) : "<i>Never indexed</i>"); ?></td>
		</tr><?php
	    }

	    ?></tbody>
	</table>
	<div class="tablenav"><input type="submit" class="<?php wt_button_class(); ?>" value="Index Selected Trails" /></div><br class="clear" />
	</form><?php
	}
    }
/* deprecated
    public function trailMeme_print_analytics_form() {
	if (!WordTrailsGlobal::get_option(WordTrailsGlobal::TRAILMEME_KEY_OPTION)) {
	    echo "You must register with Trailmeme before you can send your usage statistics.";
	} else {

	}
    }
*/

    public function trailMeme_usage_analytics_form() {
	if (!WordTrailsGlobal::get_option(WordTrailsGlobal::TRAILMEME_KEY_OPTION)) {
	    echo "You must register with Trailmeme before you can send your usage statistics.";
	} else {
	    echo "Trailmeme is not accepting following analytics at this time.";
	}
    }


    public function store_site_key($args) {
	extract($args);
	if (!isset($key) || empty($key)) return false;
	WordTrailsGlobal::update_option(WordTrailsGlobal::TRAILMEME_KEY_OPTION, $key);
    }

    public function delete_site_key($args) {
	WordTrailsGlobal::update_option(WordTrailsGlobal::TRAILMEME_KEY_OPTION, null);
    }

    public function analyticsMenu() {
	global $wpdb;
	$start = microtime(true);
	echo "<div class=\"hidden\"><div class=\"loading\"></div><div class=\"waiting\"></div></div>";
	if (isset($_GET["mode"])) {
	    if (false !== array_search($_GET["mode"], array_keys($this->modes))) {
		call_user_func(array($this, $this->modes[$_GET["mode"]]));
	    }
	}
	if (!$this->continueMenu) {
	    echo "Generated in " . (microtime(true) - $start) ." seconds.";
	    return;
	}
	
	if (!WordTrailsGlobal::get_option(WordTrailsGlobal::ANALYTICS_COLLECT_OPTION) == 1) { ?>

	<div class="wrap">
	    <h2>Collection of annonymous trail usage data is disabled</h2>
	    <form name="enable_analytics" action="<?php echo $this->href_to_submit(); ?>" method="post">
		<input type="hidden" name="process[enable_analytics][func]" value="enable_analytics" />
		<input type="hidden" name="process[enable_analytics][args][something]" value="nothing" />
		<input type="submit" value="Enable Anayltics" class="<?php wt_button_class(); ?>" /> (will also send analytics data to Trailmeme)
	    </form>
	</div><?php
	    return;
	}
	?>

	<div class="wrap">
	    <h2 style="border-bottom:none">Trail Usage &amp; Print Data</h2>
	    <div class="alignleft" style="font-size: 12px;">
		<ul class="subsubsub" style="display:inline;">
		    <li><a href="javascript:void(0)" onclick="jQuery('.expand-link').forceShow();">Show All Details</a> |</li>
		    <li><a href="javascript:void(0)" onclick="jQuery('.expand-link').forceHide();">Hide All Details</a></li>
		</ul>
	    </div>
	    <?php $this->displayUsageFor(); ?>
	    <div style="padding-top:15px;">
	    <form name="disable_analytics" action="<?php echo $this->href_to_submit(); ?>" method="post">
		<input type="hidden" name="process[disable_analytics][func]" value="disable_analytics" />
		<input type="hidden" name="process[disable_analytics][args][something]" value="nothing" />
		<input type="submit" value="Disable Anayltics" class="<?php wt_button_class(); ?>" />
	    </form>
	    </div>
	    Generated in <?php echo (microtime(true) - $start); ?> seconds.
	</div>
<script language="JavaScript" type="text/javascript">
<!--
    var ajax_url = "<?php echo admin_url("admin-ajax.php"); ?>";
    // show things that should be visible, hide what should be hidden
    jQuery('.hide-if-no-js').show();
    jQuery('.hide-if-js').hide();
//-->
</script><?php
    }

    public function single_trail_analytics() {
	if (!isset($_GET["trail"]) || empty($_GET["trail"])) return;
	if (!WordTrailsUtilities::isHash($_GET["trail"])) return;
	$siteHash = WordTrailsGlobal::get_option(WordTrailsGlobal::SITE_HASH_OPTION);
	?>

	<div class="wrap">
	    <h2 style="border-bottom:none">Usage & Print Data for<?php echo ($_GET["trail"] == $siteHash ? ": <i>{Trail Index}</i>" : " trail: " . WordTrailsGlobal::getNodeNameFromHash($_GET["trail"])); ?></h2>
<?php //$this->printMessages(); ?>
	    <div class="alignleft" style="font-size: 12px;">
		<ul class="subsubsub" style="display:inline;">
		    <li><a href="javascript:void(0)" onclick="jQuery('.expand-link').forceShow();">Show All Details</a> |</li>
		    <li><a href="javascript:void(0)" onclick="jQuery('.expand-row').show().end().find('.expand-link').click();">Hide All Details</a></li>
		</ul>
	    </div>
	    <?php $this->listSIDsForTrail($_GET["trail"]); ?>

	    <h4><a href="<?php echo $this->build_href(null, array("mode", "sid", "trail")); ?>">&larr; Back to Full Analytics</a></h4>
	</div>
<script language="JavaScript" type="text/javascript">
<!--
    var ajax_url = "<?php bloginfo( 'wpurl' ); ?>/wp-admin/admin-ajax.php";
    // show things that should be visible, hide what should be hidden
    jQuery('.hide-if-no-js').show();
    jQuery('.hide-if-js').hide();
//-->
</script><?php
	$this->continueMenu = false;
    }

    public function single_sid_analytics() {
	if (!isset($_GET["sid"]) || empty($_GET["sid"])) return;
	//trigger_error("get all details for {$_GET["sid"]}");
	$siteHash = WordTrailsGlobal::get_option(WordTrailsGlobal::SITE_HASH_OPTION);
	$hashes = WordTrailsAnalytics::trailsFromSession($_GET["sid"]);
	//trigger_error("got hashes");
	?>

	<div class="wrap">
	    <h2 style="border-bottom:none">Usage & Print Data for session: <?php echo $_GET["sid"]; ?></h2>
	    <?php
		foreach ($hashes as $trail) { ?>

	    <h3><?php echo ($trail == $siteHash ? "<i>{Trail Index}</i>" : WordTrailsGlobal::getNodeNameFromHash($trail)); ?></h3>
	    <?php
		//$this->printMessages();
		$this->detailedHitAndPrint($trail, $_GET["sid"]);

		}
	    ?>

	    <h4><a href="<?php echo $this->build_href(null, array("mode", "sid", "trail")); ?>">&larr; Back to Full Analytics</a></h4>
	</div>
<script language="JavaScript" type="text/javascript">
<!--
    // show things that should be visible, hide what should be hidden
    jQuery('.hide-if-no-js').show();
    jQuery('.hide-if-js').hide();
//-->
</script><?php
	$this->continueMenu = false;
    }

    public function detailedHitAndPrint($trail, $sid) {
	$fulldata = WordTrailsAnalytics::pullDetailsFor($trail, $sid);
	if (!isset($fulldata[$trail])) return null;
	if (!isset($fulldata[$trail][$sid])) return null;
	if (!isset($fulldata[$trail][$sid]["hit"]) && !isset($fulldata[$trail][$sid]["print"])) return null;
	if (empty($fulldata[$trail][$sid]["hit"]) && empty($fulldata[$trail][$sid]["print"])) return null;
	$data = array_merge((array)$fulldata[$trail][$sid]["hit"], (array)$fulldata[$trail][$sid]["print"]);
	@ksort($data, SORT_NUMERIC);
	
	$siteHash = WordTrailsGlobal::get_option(WordTrailsGlobal::SITE_HASH_OPTION);
	?>
	<table class="widefat" cellspacing="4">
	    <thead>
	    <tr valign="top" class="thead">
		<th scope="col"><?php _e( 'Time', WordTrailsGlobal::DOMAIN ); ?></th>
		<th scope="col"><?php _e( 'Where', WordTrailsGlobal::DOMAIN ); ?></th>
		<th scope="col"><?php _e( 'How', WordTrailsGlobal::DOMAIN ); ?></th>
	    </tr>
	    </thead>
	    <tbody><?php
	$alternate = false;
	foreach ($data as $stamp => &$hit) { ?>

	    <tr<?php echo $alternate ? " class='alternate'":"";?>>
		<td><?php echo WordTrailsUtilities::stampToAbbr($hit->Stamp); ?></td><?php
	    if (isset($hit->PrintID)) { ?>

		<td colspan="2">Printed From <?php echo WordTrailsAnalytics::$print_buttons[(int)$hit->ButtonID]; ?></td><?php
	    } elseif (isset($hit->UsageID)) {
		if ($hit->Type == WordTrailsAnalytics::WEV && isset($hit->NodeHash))
		    $where = "WEV: " . WordTrailsGlobal::getNodeNameFromHash($hit->NodeHash);
		elseif ($trail == $siteHash)
		    $where = "Trail Index";
		else
		    $where = "Trail Map";
		switch ($hit->How) {
		    case WordTrailsAnalytics::INDIRECT:
			$how = "Intersection";
			break;
		    case WordTrailsAnalytics::CHANCE:
			$how = "By Chance";
			break;
		    default:
			$how = "Direct Link";
			break;
		}
		?>

		<td><?php echo $where; ?></td>
		<td><?php echo $how; ?></td><?php
	    } else { ?>

		<td colspan="2" align="center"><i>Unknown Data</i></td><?php
	    } ?>

	    </tr><?php
	    $alternate = !$alternate;
	} ?>

	    </tbody>
	</table><?php
    }

    public function listSIDsForTrail($trail) {
	global $wp_wt_sort;
	list($crunchdata, $ignore) = WordTrailsAnalytics::pullMidLevelForTrails($trail);
	$data = &$crunchdata[$trail];
        if (!is_array($data)) $data = array();
	$siteHash = WordTrailsGlobal::get_option(WordTrailsGlobal::SITE_HASH_OPTION);
	$sort_before = $wp_wt_sort;
	if (isset($_REQUEST["wt_sid_sort"]) && !empty($_REQUEST["wt_sid_sort"]))
	    $wp_wt_sort = $_REQUEST["wt_sid_sort"];
	else $wp_wt_sort = "sid";
	$reverse = false;
	if (stristr($wp_wt_sort, "_R")) {
	    $reverse = true;
	    $wp_wt_sort = substr($wp_wt_sort, 0, -2);
	}
	if ($wp_wt_sort == "sid")
	    @ksort($data);
	else
	    @uasort($data, array("WordTrailsUtilities", "array_multi_sort"));
	if ($reverse)
	    $data = array_reverse($data);

	$spp = "sessions_per_page";
	$sp = "sessions_paged";
	$total = count($data);
	$maxcount = (int)$_REQUEST[$spp]>0 ? (int)$_REQUEST[$spp]:10;
	$page = (int)$_REQUEST[$sp]>0 ? (int)$_REQUEST[$sp]:1;
	$start = ($page-1)*$maxcount;
	while ($start > $total) {
	    $start -= $maxcount;
	    $page--;
	}
	$page_links = paginate_links( array(
	    'base' => add_query_arg( $sp, '%#%', $this->build_href(array("page"=>$this->menus["Analytics"]["slug"])) ),
	    'format' => '',
	    'total' => ceil($total/$maxcount),
	    'current' => $page
	));
	$this->num_per_page($spp, 10, $sp, "left");
	$data = array_slice($data, $start, $maxcount);
	if ($page_links) { ?>
	<div class="tablenav">
	    <div class='tablenav-pages'><?php echo $page_links; ?></div>
	</div>
	<br class="clear" /><?php
	} ?>

	<table class="widefat" cellspacing="4">
	    <thead>
	    <tr valign="top" class="thead">
		<th scope="col" class="num"><a href="<?php echo $this->build_href(array("page"=>$this->menus["Analytics"]["slug"], "wt_sid_sort"=>"sid".($wp_wt_sort == "sid" && !$reverse ? "_R" : "")));?>"><?php _e( 'SessionID', WordTrailsGlobal::DOMAIN ); ?></a></th>
		<th scope="col" class="num"><a href="<?php echo $this->build_href(array("page"=>$this->menus["Analytics"]["slug"], "wt_sid_sort"=>"WEVHits".($wp_wt_sort == "WEVHits" && $reverse ? "" : "_R")));?>"><?php _e( '# of Hits', WordTrailsGlobal::DOMAIN ); ?></a></th>
		<th scope="col"><a href="<?php echo $this->build_href(array("page"=>$this->menus["Analytics"]["slug"], "wt_sid_sort"=>"EnterHit".($wp_wt_sort == "EnterHit" && !$reverse ? "_R" : "")));?>"><?php _e( 'Entry Point', WordTrailsGlobal::DOMAIN ); ?></a></th>
		<th scope="col"><a href="<?php echo $this->build_href(array("page"=>$this->menus["Analytics"]["slug"], "wt_sid_sort"=>"LastHit".($wp_wt_sort == "LastHit" && !$reverse ? "_R" : "")));?>"><?php _e( 'Last Visited', WordTrailsGlobal::DOMAIN ); ?></a></th>
		<th scope="col"><a href="<?php echo $this->build_href(array("page"=>$this->menus["Analytics"]["slug"], "wt_sid_sort"=>"Prints".($wp_wt_sort == "Prints" && !$reverse ? "_R" : "")));?>"><?php _e( 'Print?', WordTrailsGlobal::DOMAIN ); ?></a></th>
		<th scope="col"><?php _e( 'Details', WordTrailsGlobal::DOMAIN ); ?></th>
	    </tr>
	    </thead>
	    <tbody><?php
	$alternate = true;
        if (!empty($data)):
	$expanded = (array)WordTrailsGlobal::get_option(WordTrailsGlobal::ANALYTICS_EXPANDED_OPTION);
	foreach ($data as $sid => $info) {
	    $alternate = !$alternate;
	    $exp = in_array("single_sid_stats_{$sid}_{$trail}", $expanded);?>

	    <tr<?php echo $alternate ? " class='alternate'" : ""; ?>>
		<th scope="row" class="num"><a href="<?php echo $this->build_href(array("page"=>$this->menus["Analytics"]["slug"], "mode"=>"sid_analytics", "sid"=>$sid)); ?>"><?php echo $sid; ?></a></th>
		<td class="num"><?php echo $info["WEVHits"]; ?> Hits</td>
		<td><?php echo (WordTrailsUtilities::isHash($info["EnterHit"]) ? WordTrailsGlobal::getNodeNameFromHash($info["EnterHit"]) : $info["EnterHit"]); ?></td>
		<td><?php echo (WordTrailsUtilities::isHash($info["LastHit"]) ? WordTrailsGlobal::getNodeNameFromHash($info["LastHit"]) : $info["LastHit"]); ?></td>
		<td><?php echo $info["Prints"] > 0 ? "Yes ({$info["Prints"]})" : "No"; ?></td>
		<td><a href="javascript:void(0);" class="expand-link">Show/Hide Details</a></td>
	    </tr>
	    <tr id="single_sid_stats_<?php echo "{$sid}_{$trail}"; ?>" class="expand-row hide-if-js">
		<td id="detail_trail_<?php echo "{$trail}_sid_{$sid}"; ?>" colspan="6" align="left" valign="top" class="empty<?php echo $exp ? " autoload3":"";?>">
<?php //if ($exp) $this->detailedHitAndPrint($trail, $sid); ?>
		</td>
	    </tr><?php
	}
	$wp_wt_sort = $sort_before;
        else:
        ?>

            <tr>
                <td colspan="6"><i>No analytics data for this trail</i></td>
            </tr><?php
        endif;
        ?>

	    </tbody>
	</table><?php
	if ($page_links) { ?>
	<div class="tablenav">
	    <div class='tablenav-pages'><?php echo $page_links; ?></div>
	</div>
	<br class="clear" /><?php
	}
    }

    public function displayUsageFor($trails = array()) {
	$data = array();
	if (WordTrailsAnalytics::uncrunchedGreaterThan(50)) {
	    //trigger_error("too much to crunch - switching to missiles");
	    if (empty($trails)) {
		$trails = WordTrailsGlobal::getAllTrailHashes();
		array_push($trails, WordTrailsGlobal::get_option(WordTrailsGlobal::SITE_HASH_OPTION));
	    }
	    usort($trails, array("WordTrailsNode", "nsort_name"));
	    $total = count($trails);
	} else {
	    global $wp_wt_sort;
	    $data = WordTrailsAnalytics::pullHighLevelForTrails($trails);
	    $siteHash = WordTrailsGlobal::get_option(WordTrailsGlobal::SITE_HASH_OPTION);
	    $sort_before = $wp_wt_sort;
	    if (isset($_GET["wt_trail_sort"]) && !empty($_GET["wt_trail_sort"]))
		$wp_wt_sort = $_GET["wt_trail_sort"];
	    else $wp_wt_sort = "TrailName";
	    $reverse = false;
	    if (stristr($wp_wt_sort, "_R")) {
		$reverse = true;
		$wp_wt_sort = substr($wp_wt_sort, 0, -2);
	    }
	    uasort($data, array("WordTrailsUtilities", "array_multi_sort"));
	    if ($reverse)
		$data = array_reverse($data);
	}
	$tpp = "trails_per_page";
	$tp = "trails_paged";
	$total = count($data);
	$maxcount = (int)$_GET[$tpp]>0 ? (int)$_GET[$tpp]:10;
	$page = (int)$_GET[$tp]>0 ? (int)$_GET[$tp]:1;
	$start = ($page-1)*$maxcount;
	while ($start > $total) {
	    $start -= $maxcount;
	    $page--;
	}
	$page_links = paginate_links( array(
	    'base' => add_query_arg( $tp, '%#%' ),
	    'format' => '',
	    'total' => ceil($total/$maxcount),
	    'current' => $page
	));
	$this->num_per_page($tpp, 10, $tp);
	$data = array_slice($data, $start, $maxcount);
	if ($page_links) { ?>
	<div class="tablenav">
	    <div class='tablenav-pages'><?php echo $page_links; ?></div>
	</div>
	<br class="clear" /><?php
	} ?>

	<table class="widefat" cellspacing="4">
	    <thead>
	    <tr valign="top" class="thead">
		<th scope="col"><a href="<?php echo $this->build_href(array("wt_trail_sort"=>"TrailName".($wp_wt_sort == "TrailName" && !$reverse ? "_R" : "")));?>"><?php _e( 'Trail Name', WordTrailsGlobal::DOMAIN ); ?></a></th>
		<th scope="col" class="num"><a href="<?php echo $this->build_href(array("wt_trail_sort"=>"Followers".($wp_wt_sort == "Followers" && $reverse ? "" : "_R")));?>"><?php _e( '# of Unique Walkers', WordTrailsGlobal::DOMAIN ); ?></a></th>
		<th scope="col" class="num"><a href="<?php echo $this->build_href(array("wt_trail_sort"=>"PercentFollowed".($wp_wt_sort == "PercentFollowed" && $reverse ? "" : "_R")));?>"><?php _e( 'Average Walk Depth', WordTrailsGlobal::DOMAIN ); ?></a></th>
		<th scope="col" class="num"><a href="<?php echo $this->build_href(array("wt_trail_sort"=>"Prints".($wp_wt_sort == "Prints" && $reverse ? "" : "_R")));?>"><?php _e( '# of Prints', WordTrailsGlobal::DOMAIN ); ?></a></th>
		<th scope="col"><?php _e( 'Details', WordTrailsGlobal::DOMAIN ); ?></th>
	    </tr>
	    </thead>
	    <tbody><?php
	$expanded = (array)WordTrailsGlobal::get_option(WordTrailsGlobal::ANALYTICS_EXPANDED_OPTION);
	$alternate = true;
	if (!empty($data)) {
	    foreach ($data as $hash => $info) {
		$alternate = !$alternate;
		$tooltip = array();
		foreach (array("PercentFollowed","AvgDepth","TrailCount","PercentDefaultFollowed","AvgDefaultDepth","DefaultOnlyCount") as $key) {
		    $tooltip[] = "&quot;{$key}&quot;:" . $info[$key];
		}
		$tooltip = join(",",$tooltip);
		$exp = in_array("single_trail_stats_{$hash}", $expanded); ?>
	
	    <tr<?php echo $alternate==true ? " class='alternate'" : ""; ?>><?php
		if (false === $info || empty($info)) { ?>

		<th scope="row"><?php echo WordTrailsGlobal::getNodeNameFromHash($hash); ?></th>
		<td class="num"><i>No Usage Or Print Data</i></td>
		<td colspan="3"></td>
	    </tr><?php
		    continue;
		} ?>

		<th scope="row"><a href="<?php echo $this->build_href(array("mode"=>"trail_analytics", "trail"=>$hash)); ?>"><?php echo $info["TrailName"]; ?></a></th>
		<td class="num"><?php echo $info["Followers"]; ?></td>
		<td class="num"><div class="tipthis" title="{<?php echo $tooltip; ?>}"><?php echo $info["PercentFollowed"]; ?>%</div></td>
		<td class="num"><?php echo $info["Prints"]; ?></td>
		<td><a href="javascript:void(0);" class="expand-link">Show/Hide Details</a></td>
	    </tr>
	    <tr id="single_trail_stats_<?php echo $hash; ?>" class="expand-row hide-if-js">
		<td colspan="5"id="mid_level_trail_<?php echo $hash; ?>" class="empty<?php echo $exp ? " autoload2":"";?>">
<?php //if ($exp) $this->listSIDsForTrail($hash); ?>
		</td>
	    </tr><?php
	    }
	} else {
	    foreach ($trails as $hash) {
		$alternate = !$alternate;
		$exp = in_array("single_trail_stats_{$hash}", $expanded); ?>

	    <tr id="high_level_trail_<?php echo $hash; ?>" class="autoload1<?php echo $alternate==true ? " alternate" : ""; ?>">
		<th scope="row"><?php echo WordTrailsGlobal::getNodeNameFromHash($hash); ?></th>
		<td class="num" id="status"></td>
		<td colspan="3"></td>
	    </tr>
	    <tr id="single_trail_stats_<?php echo $hash; ?>" class="expand-row hide-if-js">
		<td colspan="5" id="mid_level_trail_<?php echo $hash; ?>" class="empty<?php echo $exp ? " autoload2":""; ?>"></td>
	    </tr><?php
	   
	    }
	}
	$wp_wt_sort = $sort_before; ?>

	    </tbody>
	</table><?php
	if ($page_links) { ?>
	<div class="tablenav">
	    <div class='tablenav-pages'><?php echo $page_links; ?></div>
	</div>
	<br class="clear" /><?php
	}
    }
    
    function high_level_row($hash, &$data = null) {
	if (is_null($data)) $data = WordTrailsAnalytics::pullHighLevelForTrails($hash);
	$info = &$data[$hash];
	$tooltip = array();
	foreach (array("PercentFollowed","AvgDepth","TrailCount","PercentDefaultFollowed","AvgDefaultDepth","DefaultOnlyCount") as $key) {
	    $tooltip[] = "&quot;{$key}&quot;:" . $info[$key];
	}
	$tooltip = join(",",$tooltip);
	if (false === $info || empty($info)) { ?>

	    <th scope="row"><?php echo WordTrailsGlobal::getNodeNameFromHash($hash); ?></th>
	    <td class="num"><i>No Usage Or Print Data</i></td>
	    <td colspan="3"></td>
	<?php
	    return;
	} ?>

	    <th scope="row"><a href="<?php echo $this->build_href(array("page"=>$this->menus["Analytics"]["slug"], "mode"=>"trail_analytics", "trail"=>$hash)); ?>"><?php echo $info["TrailName"]; ?></a></th>
	    <td class="num"><?php echo $info["Followers"]; ?></td>
	    <td class="num"><div class="tipthis" title="{<?php echo $tooltip; ?>}"><?php echo $info["PercentFollowed"]; ?>%</div></td>
	    <td class="num"><?php echo $info["Prints"]; ?></td>
	    <td><a href="javascript:void(0);" class="expand-link">Show/Hide Details</a></td>
	<?php
    }

    public function trail_page_form() {
	global $wpdb;

	$pid = WordTrailsGlobal::get_option(WordTrailsGlobal::TRAILS_PAGE_OPTION);
	$title = null;
	$query = array();
	if (is_numeric($pid)) $query = query_posts("page_id=" . $pid);
	if (empty($query)) {
	    $pid = null;
	    WordTrailsGlobal::update_option(WordTrailsGlobal::TRAILS_PAGE_OPTION, null);
	}
	if (is_numeric($pid)) {
	    $title = get_the_title((int)$pid);
	    ?>

    <form name="remove_current_trails_page" id="remove_current_trails_page" action="<?php echo $this->href_to_submit(); ?>" method="post">
    <input type="hidden" name="process[remove_current][func]" value="" id="remove_current" />
    <input type="hidden" name="process[remove_current][args][confirm]" value="false" />
    <h2 style="border-bottom:none; margin:0px;">Current Display Page: <strong><em><?php echo $title; ?></em></strong></h2>
    <input type="button" value="Disassociate With Page" class="<?php wt_button_class(); ?>"  onclick="jQuery('#remove_current').attr('value', 'disassociate_current_trails_page');jQuery('#remove_current_trails_page').submit();" />
    <input type="button" value="Delete Page" class="<?php wt_button_class(); ?>" onclick="jQuery('#remove_current').attr('value', 'delete_current_trails_page');jQuery('#remove_current_trails_page').submit();" />
    </form><?php
	} ?>
    <h2 style="border-bottom:none; margin-bottom:0px;">New Trails Display Page</h2>
    <form name="new_trails_page" id="new_trails_page" action="<?php echo $this->href_to_submit(); ?>" method="post">
    <input type="hidden" name="process[new_trails_page][func]" value="new_trails_page" />
    <label>Page Name or ID: <input type="text" name="process[new_trails_page][args][input]" id="new_trails_page_name" value="Trails" onkeyup="checkNewTrailsPage();" /></label>
    <input type="submit" class="<?php wt_button_class(); ?>" id="new_trails_submit" value="Create New Page to display Trails" disabled="disabled" /><br />
    <small>Enter the name or ID of an existing page, or enter the name for a new page (Default = "Trails"). </small>
    </form><?php
    }

    public function new_trails_page($args) {
	extract($args);
	if (is_null($input)) return false;
	if (is_numeric($input))
	    $query = query_posts("page_id=" . $input);
	else
	    $query = query_posts("pagename=" . $input);
	if (!empty($query)) {
	    $post_data = array();
	    $post_data["post_ID"] = $query[0]->ID;
	    $post_data["post_type"] = "page";
	    $post_data['comment_status'] = 'closed';
	    $post_data['ping_status'] = 'closed';
	    $post_data['content'] = __('These are trails to help you navigate the contents of this blog. Click on any trail to see a map, or just start following.', WordTrailsGlobal::DOMAIN);
	    edit_post($post_data);
	    WordTrailsGlobal::update_option(WordTrailsGlobal::TRAILS_PAGE_OPTION, $query[0]->ID);
	    WordTrailsUtilities::getHijackTitle(true);
	} else {
	    $_POST['post_date'] = '0000-00-00 00:00:00';
	    $_POST['post_date_gmt'] = '0000-00-00 00:00:00';
	    $_POST['post_type'] = "page";
	    $_POST['post_status'] = 'publish';
	    $_POST['comment_status'] = 'closed';
	    $_POST['ping_status'] = 'closed';
	    $_POST['post_title'] = $input;
	    $_POST['content'] = __('These are trails to help you navigate the contents of this blog. Click on any trail to see a map, or just start following.', WordTrailsGlobal::DOMAIN);
	    $id = write_post();
	    if (is_numeric($id)) {
		WordTrailsGlobal::update_option(WordTrailsGlobal::TRAILS_PAGE_OPTION, $id);
		WordTrailsUtilities::getHijackTitle(true);
	    }
	}
	global $wp_rewrite;
	$wp_rewrite->flush_rules();
    }

    public function disassociate_current_trails_page($args) {
	$pid = WordTrailsGlobal::get_option(WordTrailsGlobal::TRAILS_PAGE_OPTION);
	WordTrailsGlobal::update_option(WordTrailsGlobal::TRAILS_PAGE_OPTION, null);
	global $wp_rewrite;
	$wp_rewrite->flush_rules();
	if (is_numeric($pid))
	    return $pid;
    }

    public function delete_current_trails_page($args) {
	extract($args);
	if ($confirm != "false") {
	    if (stristr($confirm, "yes")) {
		$this->continueMenu = false;
		$pid = $this->disassociate_current_trails_page(true);
	        //delete it too
		//echo "<div class='message'>" . clean_url(wp_nonce_url(get_option("home") . "/wp-admin/post.php?action=delete&post=" . $pid, "delete")) . "</div>";
		if (is_numeric($pid))
		    header("Location: " . get_option("home") . "/wp-admin/page.php?action=delete&post=" . $pid . "&_wpnonce=" . wp_create_nonce('delete-page_' . $pid));
	    }
	} else {
	    $title = "Are you sure you want to delete the entire page? This process cannot be undone.";
	    $msg = "<form name=\"delete_confirm\" action=\"" . $this->href_to_submit() . "\" method=\"post\">
	<input type=\"hidden\" name=\"process[delete_trails_page][func]\" value=\"delete_current_trails_page\" />
	<input type=\"submit\" class=\"{$wt_button_class}\" name=\"process[delete_trails_page][args][confirm]\" value=\"Yes - Delete the entire page\" />
	<input type=\"submit\" class=\"{$wt_button_class}\" name=\"process[delete_trails_page][args][confirm]\" value=\"No\" />
	</form>";
	$this->addMessage($msg, "delete_page_confirm_notice", $title);
	$this->continueMenu = false;
	}
    }
    
    
    public function helpMenu() {
	add_meta_box("help_about", "About", array($this, "helpAbout"), "help");
	add_meta_box("help_features", "Upcoming Features", array($this, "helpFeatures"), "help");
	//add_meta_box("help_faq", "FAQ", array($this, "helpFAQ"), "help");
	//add_meta_box("help_analytics", "Analytics Details", array($this, "helpAnalytics"), "help");
	add_meta_box("help_rss", "blog.trailmeme.com RSS Feed <a href='http://blog.trailmeme.com/feed/' title='Subscribe'><img src='" . WordTrailsGlobal::$urlpath . "imgs/feed-icon-14x14.png' alt='RSS Feed' title='Subscribe' /></a>", array($this, "helpRSS"), "help");
	
	?>

<div class="wrap">
    <h2 style="border-bottom:none"><?php _e( 'Trailmeme for Wordpress: More Information', WordTrailsGlobal::DOMAIN ); ?></h2>
<div id="poststuff">

<?php
do_meta_boxes("help", 'advanced', false);
wp_nonce_field( 'closedpostboxes', 'closedpostboxesnonce', false );
wp_nonce_field( 'meta-box-order', 'meta-box-order-nonce', false );
?>

</div>
</div>
<script language="JavaScript" type="text/javascript">
<!--

    var ajax_url = "<?php bloginfo( 'wpurl' ); ?>/wp-admin/admin-ajax.php";

    <?php post_box_toggles("help"); ?>

    // close postboxes that should be closed
    jQuery('.if-js-closed').removeClass('if-js-closed').addClass('closed');

    // show things that should be visible, hide what should be hidden
    jQuery('.hide-if-no-js').show();
    jQuery('.hide-if-js').hide();

//-->
</script><?php
    }
    
    public function helpAbout() { ?>
<p>Wouldn't you like to offer your readers a "best of my blog" feature that is easy for you to maintain? Or maybe a series of posts that add up to a novel? Or a tutorial on your area of expertise?</p>
<p><img src="<?php echo WordTrailsGlobal::$urlpath; ?>imgs/logos/Trailmeme_Logo_100x100.jpg" alt="Trailmeme Logo" class="alignleft" />You can do all that and more with Trailmeme for Wordpress, one of the products in the Trailmeme ecosystem. Trailmeme for Wordpress allows you to blaze trails on your WordPress blog that users can easily navigate with a sidebar widget and a Flash map. The benefits are:
<ol class="small" style="margin-left: 130px;">
<li><a href="http://blog.trailmeme.com/2009/12/drive-traffic-using-trailmeme-for-wordpress/">Drive traffic and increase session length</a></li>
<li><a href="http://blog.trailmeme.com/2009/12/print-enabling-your-blog-with-trailmeme/">Allow your readers to create instant PDFs</a></li>
<li><a href="http://blog.trailmeme.com/2009/12/understanding-trailmeme-analytics/">Get real-time analytics that tell you which trails are being followed</a></li>
</ol></p><br clear="all" />
<p>There are many other benefits. You may want to subscribe to the <a href="http://blog.trailmeme.com">blog.trailmeme.com</a> for hints, tips, updates and coaching. You will also find more extensive tutorials there (organized as trails of course!)</p>
<p>You can also register your plugin with trailmeme.com and index your blog's trails there for easy searchability/discovery. And you can create trails across arbitrary pages on trailmeme.com and feature them on your blog. So for instance, you can do a trail every week through the blogs on your blogroll and feature it as a "best of what I read this week."</p>
<p>Watch this page in future versions, we'll add more useful information as we go along. For now, if you need help, just email <a href="mailto:jesse.silverstein@xerox.com">jesse.silverstein@xerox.com</a> or post a comment on <a href="http://blog.trailmeme.com">blog.trailmeme.com</a></p>
	<?php
    }
    
    public function helpRSS() {
	echo '<p class="widget-loading hide-if-no-js">' . __( 'Loading&#8230;' ) . '</p><p class="describe hide-if-js">' . __('This widget requires JavaScript.') . '</p>';
    }
    
    public function helpFeatures() { ?>
	<h4>Here is a snapshot of upcoming features in Trailmeme for WordPress!</h4>
	<ul style="list-style:inside disc;">
	    <li>Trail Map snap-to-grid during editing - line those nodes up!</li>
	    <li>Support for non-widget enabled themes - php code snippet to add to your theme files, if you do not want to use the widget</li>
	    <li>Internationalization</li>
	</ul>
	<h4>For more upcoming features, or to submit a Feature Request, check out the <a href="http://blog.trailmeme.com/roadmap/" target="_blank">Roadmap</a></h4>
<?php
    }

}
?>