jQuery(document).ready(function() {
    jQuery(".ltoh").hover(function() {
        $img = jQuery(this).find("img");
        var src = $img.attr("src");
        var newsrc = src.replace(/_[lh]/, "_h");
        $img.attr("src", newsrc);
    }, function() {
        $img = jQuery(this).find("img");
        var src = $img.attr("src");
        var newsrc = src.replace(/_[lh]/, "_l");
        $img.attr("src", newsrc);
    }).each(function() {
        $img = jQuery(this).find("img");
        var src = $img.attr("src");
        var newsrc = src.replace(/_[lh]/, "_h");
        jQuery("<img>").attr("src", newsrc);
    });
    jQuery("a[rel=external]").attr("target", "_blank");
});