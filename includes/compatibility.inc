<?php

//backwards compatible location definitions for 2.5.x
if ( !defined( 'WP_CONTENT_URL' ) )
    define( 'WP_CONTENT_URL', get_option( 'siteurl' ) . '/wp-content' );
if ( !defined( 'WP_CONTENT_DIR' ) )
    define( 'WP_CONTENT_DIR', ABSPATH . 'wp-content' );
if ( !defined( 'WP_PLUGIN_URL' ) )
    define( 'WP_PLUGIN_URL', WP_CONTENT_URL . '/plugins' );
if ( !defined( 'WP_PLUGIN_DIR' ) )
    define( 'WP_PLUGIN_DIR' , WP_CONTENT_DIR . '/plugins' );

function wt_menu_prereqs() {
    wp_register_script("ltoh", WordTrailsGlobal::$urlpath . "js/ltoh.js", array("jquery"));
    $prereqs = array('postbox', 'ltoh');
    global $wp_version;
    if(version_compare($wp_version,"2.7-alpha", "<"))
	array_push($prereqs, 'admin-forms');
    return $prereqs;
}

function post_box_toggles($trigger) {
    global $wp_version;
    if(version_compare($wp_version,"2.7-alpha", "<")) {
	echo "add_postbox_toggles(\"$trigger\");"; //For WP2.6 and below
    } else {
	echo "postboxes.add_postbox_toggles(\"$trigger\");"; //For WP2.7 and above
    }
}

function wt_button_class($return = false) {
    global $wp_version;
    $class = "";
    if(version_compare($wp_version,"2.7-alpha", "<"))
	$class = "button-secondary";
    else
	$class = "button-primary";
    if ($return)
	return $class;
    else
	echo $class;
}

function wt_alt_button_class($return = false) {
    global $wp_version;
    $class = "";
    if(version_compare($wp_version,"2.7-alpha", "<"))
	$class = "button";
    else
	$class = "button-secondary";
    if ($return)
	return $class;
    else
	echo $class;
}

function wt_checkAll($form, $return = false) {
    global $wp_version;
    if (version_compare($wp_version, "2.7-alpha", "<")) {
	$ret = " onclick=\"checkAll({$form});\" ";
    } else
	$ret = "";
    if ($return) return $ret;
    echo $ret;
}

function wt_text_area_size($rows, $cols) {
    global $wp_version;
    if(version_compare($wp_version,"2.7-alpha", "<")) {
	$rows *= 2;
	$cols *= 2;
    }
    return " rows=\"$rows\" cols=\"$cols\" ";
}

function wt_text_box_size($size) {
    global $wp_version;
    if(version_compare($wp_version,"2.7-alpha", "<")) {
	$size *= 2;
    }
    return " size=\"$size\" ";
}

if (!function_exists("json_encode")) {
    require_once "JSON.php";
    global $json_serv_obj;
    $json_serv_obj = new Services_JSON();
    function json_encode($var) {
	global $json_serv_obj;
	return $json_serv_obj->encode($var);
    }
    function json_decode($var) {
	global $json_serv_obj;
	return $json_serv_obj->decode($var);
    }
}

if (!function_exists("admin_url")) {
    function admin_url($path='') {
	$url = get_option('siteurl').'/wp-admin/';

	if ( !empty($path) && is_string($path) && strpos($path, '..') === false )
		$url .= ltrim($path, '/');

	return apply_filters('admin_url', $url, $path);
    }
}

if (!function_exists("translate_user_role")) {
    function translate_user_role( $name ) {
	return __( before_last_bar( $name ) );
    }
}
if (!function_exists("before_last_bar")) {
    function before_last_bar( $string ) {
	$last_bar = strrpos( $string, '|' );
	if ( false == $last_bar )
		return $string;
	else
		return substr( $string, 0, $last_bar );
    }
}

if (!function_exists("wp_check_invalid_utf8")) {
    /**
     * Checks for invalid UTF8 in a string.
     *
     * @since 2.8
     *
     * @param string $string The text which is to be checked.
     * @param boolean $strip Optional. Whether to attempt to strip out invalid UTF8. Default is false.
     * @return string The checked text.
     */
    function wp_check_invalid_utf8( $string, $strip = false ) {
	$string = (string) $string;

	if ( 0 === strlen( $string ) ) {
	    return '';
	}

	// Store the site charset as a static to avoid multiple calls to get_option()
	static $is_utf8;
	if ( !isset( $is_utf8 ) ) {
	    $is_utf8 = in_array( get_option( 'blog_charset' ), array( 'utf8', 'utf-8', 'UTF8', 'UTF-8' ) );
	}
	if ( !$is_utf8 ) {
	    return $string;
	}

	// Check for support for utf8 in the installed PCRE library once and store the result in a static
	static $utf8_pcre;
	if ( !isset( $utf8_pcre ) ) {
	    $utf8_pcre = @preg_match( '/^./u', 'a' );
	}
	// We can't demand utf8 in the PCRE installation, so just return the string in those cases
	if ( !$utf8_pcre ) {
	    return $string;
	}

	// preg_match fails when it encounters invalid UTF8 in $string
	if ( 1 === @preg_match( '/^./us', $string ) ) {
	    return $string;
	}

	// Attempt to strip the bad chars if requested (not recommended)
	if ( $strip && function_exists( 'iconv' ) ) {
	    return iconv( 'utf-8', 'utf-8', $string );
	}

	return '';
    }
}
if (!function_exists("wp_strip_all_tags")) {
    /**
    * Properly strip all HTML tags including script and style
    *
    * @since 2.9.0
    *
    * @param string $string String containing HTML tags
    * @param bool $remove_breaks optional Whether to remove left over line breaks and white space chars
    * @return string The processed string.
    */
    function wp_strip_all_tags($string, $remove_breaks = false) {
	$string = preg_replace( '@<(script|style)[^>]*?>.*?</\\1>@si', '', $string );
	$string = strip_tags($string);
   
	if ( $remove_breaks )
	    $string = preg_replace('/[\r\n\t ]+/', ' ', $string);

	return trim($string);
    }
}
if (!function_exists("sanitize_text_field")) {
    /**
    * Sanitize a string from user input or from the db
    *
    * check for invalid UTF-8,
    * Convert single < characters to entity,
    * strip all tags,
    * remove line breaks, tabs and extra whitre space,
    * strip octets.
    *
    * @since 2.9
    *
    * @param string $str
    * @return string
    */
    function sanitize_text_field($str) {
	$filtered = wp_check_invalid_utf8( $str );
	if ( strpos($filtered, '<') !== false ) {
	    $filtered = wp_pre_kses_less_than( $filtered );
	    $filtered = wp_strip_all_tags( $filtered, true );
	} else {
	    $filtered = trim( preg_replace('/[\r\n\t ]+/', ' ', $filtered) );
	}

	$match = array();
	while ( preg_match('/%[a-f0-9]{2}/i', $filtered, $match) )
	$filtered = str_replace($match[0], '', $filtered);

	return apply_filters('sanitize_text_field', $filtered, $str);
   }
}


?>