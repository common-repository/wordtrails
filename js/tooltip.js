var tooltipObj = {
    track: true,
    delay: 0,
    showURL: false,
    fixPNG: true,
    extraClass: "pretty",
    top: -5,
    left: 10,
    bodyHandler: customBodyHandler
};
jQuery(document).ready(function() {
    wt_preload_tooltip();
    jQuery(".tipthis").each(function() {
        $node = jQuery(this);
        $node.tooltip(tooltipObj);
    });
});

function customBodyHandler() {
    $info = this.tooltipText;//.replace(/'/g, '"');
    $div = jQuery("<div/>").addClass("tipholder");
    $div.append(jQuery("<div/>").addClass("tip-top"));
    $wrap = jQuery("<div/>").addClass("wrap");
    if ($info != "") {
        $info = eval("(" + $info + ");");
        if ($info.Name) {
            $wrap.append(jQuery("<div>" + $info.Name + "</div>").addClass("name"));
        }
        if ($info.ShortDesc) {
            $wrap.append(jQuery("<div>" + $info.ShortDesc + "</div>").addClass("caption"));
        }
        if ($info.LongDesc) {
            $wrap.append(jQuery("<div>" + $info.LongDesc + "</div>").addClass("description"));
        }
        if ($info.tags) {
            $wrap.append(jQuery("<div>Tags: " + $info.tags.join(", ") + "</div>").addClass("tags"));
        }
        if ($info.AvgDepth != undefined && $info.AvgDefaultDepth != undefined) {
            $wrap.append(jQuery("<div>Entire Trail: " + $info.PercentFollowed + "% ("+ $info.AvgDepth + "/" + $info.TrailCount + ")</div>").addClass("stats"));
            $wrap.append(jQuery("<div>Default Path: " + $info.PercentDefaultFollowed + "% (" + $info.AvgDefaultDepth + "/" + $info.DefaultOnlyCount + ")</div>").addClass("stats"));
        }
    }
    $div.append($wrap);
    $div.append(jQuery("<div/>").addClass("tip-bottom"));
    return $div;
}