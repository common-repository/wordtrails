<?php
require_once "tooltip.inc";
function wt_hijack_content($contentorig) {
    $content = '<div id="WordTrails">';
    $trail_slug = get_query_var('trail_slug');
    $default_to_index = true;
    if (!empty($_GET['t'])) {
	/*
	 if (!empty($_GET["print"])) {
	    $print = WordTrailsUtilities::makePrintNode($_GET['t']);
	    $content .= $print->display();
	} else {
	//*/
	    $trail = WordTrailsGlobal::getNode(WordTrailsGlobal::getNodeHashFromID($_GET['t']));
	    if (is_object($trail)) {
		if (!in_the_loop()) {
		    return "Trail Map for " . $trail->getName();
		}
		$default_to_index = false;
		$content .= $trail->display();
	    }
	//}
    } elseif (!empty($trail_slug)) {
	//trigger_error("slug not empty");
	//trigger_error(print_r(WordTrailsGlobal::$instance->nodeSlugToHash, true));
	$hash = WordTrailsGlobal::getNodeHashFromSlug($trail_slug);
	if (!empty($hash)) {
	    //trigger_error("found hash: $hash");
	    $trail = WordTrailsGlobal::getNode($hash);
	    //trigger_error("tried to make trail from hash");
	    if (is_object($trail)) {
		if (!in_the_loop()) {
		    return "Trail Map for " . $trail->getName();
		}
		//trigger_error("made trail - cannot be index");
		$default_to_index = false;
		$content .= $trail->display();
	    }
	}
    }
    if ($default_to_index) {
	if (!in_the_loop()) {
	    return "Index of Trails";
	}
	global $post;
	$_post = sanitize_post($post, 'raw');
	//superprint($_post, true);
	$content .= $_post->post_content;
	/*
	$content .='<div class="TrailMeme">
	Global Trail Index <a href="http://trailmeme.com" class="ltoh"><img src="' . WordTrailsGlobal::$urlpath . 'imgs/buttons/trailmeme_l.png" alt="TrailMeme" title="Global Trail Index" /></a>
    </div>';
	*/
	$hashes = WordTrailsGlobal::getAllTrailHashes();
	if (!empty($hashes)) {
	    usort($hashes, array("WordTrailsNode","nsort_name"));
	    $ppp = (int)get_query_var('posts_per_page');
	    if (count($hashes) > $ppp) {
	        $page = max(1,(int)get_query_var('paged'));
		$start = $ppp * max(0,($page-1));
		$page_links = paginate_links( array(
		    'base' => add_query_arg( 'paged', '%#%' ),
		    'format' => '',
		    'total' => ceil(count($hashes)/$ppp),
		    'current' => $page
		));
	    }
	    $count = -1;
	    $trails = WordTrailsGlobal::getNode($hashes);
	    if ($page_links) $content .= "<div style=\"text-align:right;\">{$page_links}</div>";
	    foreach ($trails as $hash => $trail) {
		if (!is_object($trail)) continue;
		if (isset($start)) {
		    $count++;
		    if ($count < $start) continue;
		    if ($count >= $start + $ppp) break;
		}
		$content .= $trail->displayInList();
	    }
	    if ($page_links) $content .= "<div style=\"text-align:right;\">{$page_links}</div>";
	} else {
	    $content .= "<i>There are no trails to display.</i>";
	}
    }
    $content .= '
    </div>
    <div class="Trailmeme">
	<a href="http://wordpress.org/extend/plugins/wordtrails" rel="external">Get this plugin</a>. Powered by <a href="http://trailmeme.com"><img src="' . WordTrailsGlobal::$urlpath . 'imgs/logos/trailmeme.png" alt="TrailMeme" /></a>
    </div>';
    return $content;
}
add_filter("the_content", "wt_hijack_content", 100);
            
function wt_hijack_title($title, $ID = null) {
    if(is_null($ID)){
	if ($title == WordTrailsUtilities::getHijackTitle())
	    $ID = WordTrailsGlobal::get_option(WordTrailsGlobal::TRAILS_PAGE_OPTION);
        else
	    return $title;
    }
    if ($ID != WordTrailsGlobal::get_option(WordTrailsGlobal::TRAILS_PAGE_OPTION)) return $title;
    $title = "Index of Trails";
    if (!empty($_GET['t'])) {
        if (is_object(WordTrailsGlobal::getNode(WordTrailsGlobal::getNodeHashFromID($_GET['t'])))) {
            $title = "Trail Map";
            //if (!empty($_GET["print"])) $title = "Print Editor";
        }
    } elseif ($slug = get_query_var("trail_slug")) {
	if ($hash = WordTrailsGlobal::getNodeHashFromSlug($slug)) {
	    if (is_object(WordTrailsGlobal::getNode($hash)))
		$title = "Trail Map";
	}
    }
    return $title;
}
add_filter("the_title", "wt_hijack_title", 100, 2);

function wt_hijack_head() { ?>
<meta name="trailmeme-enabled" content="wordpress" />
<?php
}
add_action("wp_head", "wt_hijack_head", 0);
?>