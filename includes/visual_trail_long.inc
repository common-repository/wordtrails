<?php
if (!isset($xml))
    $xml = WordTrailsUtilities::redirect_link_to("xml");
if (!is_array($flashvars))
    $flashvars = array();
$flashvars["xml_loc"] = urlencode($xml);
//$flashvars["force_debug"] = "true";
$flashvars["nocache"] = time();
$flashvars["save_xml_var"] = "xml";
/**
 * Flash Vars:
 * xml_str - Actual XML String - overrides xml_loc
 * xml_loc - URL to get XML from
 * save_uri - Where to send the save POST
 * save_xml_var - variable name to send XML as - defaults to "xml"
 * view_only - true or false (without save_uri, defaults to true - with save_uri, defaults to false - this can override save_uri defaults)
 * ...rest - will be sent back with the POST

 */
$trailViewer = "";
if (!isset($override_backup_div)) $override_backup_div = false;
if ($override_backup_div !== true) {
	$trailViewer .= '
		<div id="TrailViewer">
			<a href="http://www.adobe.com/go/getflashplayer">
				<img src="http://www.adobe.com/images/shared/download_buttons/get_flash_player.gif" alt="Get Adobe Flash player" />
			</a>
		</div>';
}
$trailViewer .= '
		<script type="text/javascript" src="' . WordTrailsGlobal::$urlpath . 'js/swfobject.js"></script>
		<script type="text/javascript">
			var flashvars = {};';
foreach ((array)$flashvars as $var => $val) {
	$trailViewer .= '
			flashvars.' . $var . ' = "' . $val . '";';
}
$trailViewer .= '
			var params = {};
			params.play = "true";
			params.loop = "false";
			params.menu = "false";
			params.quality = "best";
			params.scale = "exactfit";
			params.allowfullscreen = "false";
			params.allowscriptaccess = "always";
			params.allownetworking = "all";
			var attributes = {};
			attributes.id = "TrailViewer";
			attributes.name = "TrailViewer";
			swfobject.embedSWF(
			    "' . WordTrailsGlobal::$urlpath . 'swf/TrailViewerLong.swf",
			    "TrailViewer",
			    "400",
			    "750",
			    "9.0.0",
			    "' . WordTrailsGlobal::$urlpath . 'swf/expressInstall.swf",
			    flashvars,
			    params,
			    attributes
			);
		</script>';
if (!isset($abort_display) || $abort_display !== true) echo $trailViewer;