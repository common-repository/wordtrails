<?php
require('fpdf.php');

class WordTrailsPDFExtender extends FPDF {
    public $prevSize;
    function __construct($orientation='P',$unit='mm',$format='A4') {
        //Call parent constructor
        $this->FPDF($orientation,$unit,$format);
        //Initialization
        $this->B=0;
        $this->I=0;
        $this->U=0;
        $this->HREF='';
        $this->SetFont('courier');
    }

    function WriteHTML($html) {
        //HTML parser
        $html=str_replace("\n",'<br>',$html);
        $html=str_replace("&nbsp;",' ',$html);
        $a=preg_split('/<(.*)>/U',$html,-1,PREG_SPLIT_DELIM_CAPTURE);
        foreach($a as $i=>$e) {
            if($i%2==0) {
                //Text
                if($this->HREF)
                    $this->PutLink($this->HREF,$e);
                else
                    $this->Write(5,$e);
            } else {
                //Tag
                if($e{0}=='/')
                    $this->CloseTag(strtoupper(substr($e,1)));
                else {
                    //Extract attributes
                    $a2=explode(' ',$e);
                    $tag=strtoupper(array_shift($a2));
                    $attr=array();
                    foreach($a2 as $v) {
                        if(preg_match('/([^=]*)=["\']?([^"\']*)/',$v,$a3))
                            $attr[strtoupper($a3[1])]=$a3[2];
                    }
                    $this->OpenTag($tag,$attr);
                }
            }
        }
    }

    function OpenTag($tag,$attr) {
        //Opening tag
        if($tag=='B' or $tag=='I' or $tag=='U')
            $this->SetStyle($tag,true);
        if($tag=='EM')
            $this->SetStyle('B',true);
        if($tag=='A')
            $this->HREF=$attr['HREF'];
        if($tag=='BR' || $tag=='P')
            $this->Ln(5);
        if($tag=='H2') {
            $this->Ln(5);
            $this->prevSize = $this->FontSize;
            $this->SetFontSize($this->prevSize+3);
            $this->SetStyle('B', true);
        }
        if($tag=='H3') {
            $this->Ln(5);
            $this->prevSize = $this->FontSize;
            $this->SetFontSize($this->prevSize+2);
            $this->SetStyle('B', true);
        }
        if ($tag=="IMG") {
            if (!isset($attr["SRC"])) return;
            if ($attr["WIDTH"] == "0" || $attr["HEIGHT"] == "0" || (int)$attr["WIDTH"] == 1 || (int)$attr["HEIGHT"] == 1) return;
            $src = $attr["SRC"];
            if ($url_parts = parse_url($attr["SRC"])) {
                if (isset($url_parts["host"]) && !empty($url_parts["host"])) {
                    // && $url_parts["host"] != parse_url(get_option('home'), PHP_URL_HOST)
                    require_once trailingslashit(ABSPATH . WPINC) . "class-snoopy.php";
                    $snoop = new Snoopy();
                    if ($snoop->fetch($src)) {
                        $mime = false;
                        //superprint($snoop->headers, true);
                        foreach ($snoop->headers as $header) {
                            if (strstr($header, "HTTP/1.1")) {
                                if (!strstr($header, "200")) {
                                    $spl = array_reverse(split("/",$src));
                                    $this->WriteHTML("<i>[Error accessing image (<a href='" . $src . "'>" . $spl[0] . "</a>): " . trim($header) . "]");
                                    return;
                                }
                            }
                            if (strstr($header, "Content-Type: image")) {
                                $mime = substr(trim($header), strpos(trim($header), " ")+1);
                                break;
                            }
                        }
                        if (!$mime) {
                            $spl = array_reverse(split("/",$src));
                            $this->WriteHTML("<i>[Unable to determine image type: <a href='" . $src . "'>" . $spl[0] . "</a>]");
                            return;
                        } else {
                            if ($tmp = tempnam("/tmp","ext_img")) {
                                if (is_writable($tmp)) {
                                    $file = fopen($tmp, "w");
                                    fwrite($file, $snoop->results);
                                    fclose($file);
                                    $src = $tmp;
                                    switch ($mime) {
                                        case "image/jpeg":
                                            $type="jpg";
                                            break;
                                        case "image/gif":
                                            $type="gif";
                                            break;
                                        case "image/png":
                                            $type="png";
                                            break;
                                    }
                                } else {
                                    $spl = array_reverse(split("/",$attr['SRC']));
                                    $this->WriteHTML("<i>[Unable to create temporary image. Source: <a href='" . $attr['SRC'] . "'>" . $spl[0] . "</a>]");
                                    return;
                                }
                            } else {
                                $spl = array_reverse(split("/",$attr['SRC']));
                                $this->WriteHTML("<i>[Unable to create temporary image. Source: <a href='" . $attr['SRC'] . "'>" . $spl[0] . "</a>]");
                                return;
                            }
                        }
                    } else {
                        $spl = array_reverse(split("/",$attr['SRC']));
                        $this->WriteHTML("<i>[Unable to access image. Source: <a href='" . $attr['SRC'] . "'>" . $spl[0] . "</a>]</i>");
                        return;
                    }
                }
            }
            if (!isset($type) && $pos = strrpos($attr['SRC'], ".")) {
                $type = substr($attr['SRC'],$pos+1);
                $type = strtolower($type);
            }
            $jpgs = array("jpg", "jpeg");
            if (isset($type) && in_array($type, $jpgs)) {
                $this->Image($src, null, null, (int)$attr["WIDTH"]/$this->k, (int)$attr["HEIGHT"]/$this->k, $type);
            } else {
                $im = false;
                switch ($type) {
                    case "jpg":
                    case "jpeg":
                        if (function_exists("imagecreatefromjpeg"))
                            $im = @imagecreatefromjpeg($src);
                        break;
                    case "png":
                        if (function_exists("imagecreatefrompng"))
                            $im = @imagecreatefrompng($src);
                        break;
                    case "gif":
                        if (function_exists("imagecreatefromgif"))
                            $im = @imagecreatefromgif($src);
                        break;
                    default:
                        if (function_exists("imagecreatefromjpeg"))
                            $im = @imagecreatefromjpeg($src);
                        if (!$im && function_exists("imagecreatefrompng"))
                            $im = @imagecreatefrompng($src);
                        if (!$im && function_exists("imagecreatefromgif"))
                            $im = @imagecreatefromgif($src);
                }
                if (!$im && function_exists("imagecreatefromjpeg"))
                    $im = @imagecreatefromjpeg($src);
                if (!$im && function_exists("imagecreatefrompng"))
                    $im = @imagecreatefrompng($src);
                if (!$im && function_exists("imagecreatefromgif"))
                    $im = @imagecreatefromgif($src);
                if (!$im) {
                    $this->WriteHTML("<i>[Unable to convert image. Source: " . $attr['SRC'] . "]</i>");
                } else {
                    imageinterlace($im,0);
                    imagealphablending($im, true);
                    imagesavealpha($im, false);
                    $tmp = tempnam("/tmp", "ext_img");
                    if ($tmp != false) {
                        imagepng($im, $tmp);
                        $this->Image($tmp, null, null, (int)$attr["WIDTH"]/$this->k, (int)$attr["HEIGHT"]/$this->k, "png");
                    }
                }
            }
            if ($tmp) {
                unlink($tmp);
            }
        }
    }

    function CloseTag($tag) {
        //Closing tag
        if($tag=='B' or $tag=='I' or $tag=='U')
            $this->SetStyle($tag,false);
        if($tag=='EM')
            $this->SetStyle('B',false);
        if($tag=='A')
            $this->HREF='';
        if($tag=='P')
            $this->Ln(5);
        if($tag=='H2' || $tag=='H3') {
            $this->Ln(5);
            $this->SetStyle('B', false);
            $this->SetFontSize($this->prevSize);
        }
    }

    function SetStyle($tag,$enable) {
        //Modify style and select corresponding font
        $this->$tag+=($enable ? 1 : -1);
        $style='';
        foreach(array('B','I','U') as $s)
            if($this->$s>0)
                $style.=$s;
        $this->SetFont('',$style);
    }

    function PutLink($URL,$txt) {
        //Put a hyperlink
        $this->SetTextColor(0,0,255);
        $this->SetStyle('U',true);
        $this->Write(5,$txt,$URL);
        $this->SetStyle('U',false);
        $this->SetTextColor(0);
    }

    function Footer() {
        //Position at 1.5 cm from bottom
        $this->SetY(-15);
        //Arial italic 8
        $this->SetFont('Arial','I',8);
        //Page number
        $this->Cell(0,10,'Page '.$this->PageNo().'/{nb}',0,0,'C');
    }


    function Title($txt) {
        $this->SetFont('Arial','B',24);
        $this->Cell(0,8,$txt,0,1,"C");
    }
    function ShortDesc($txt) {
        $this->SetFont('Arial','I',18);
        $this->Cell(0,6,$txt,0,1,"C");
    }
    function LongDesc($txt) {
        $this->SetFont('Arial', '', 14);
        $this->MultiCell(0,5,$txt,0,"C");
        $this->Ln(2);
    }

    function ContentLinks($mappedmarkers) {
        $links = array();
        $this->SetTextColor(0,0,255);
        $this->SetFontSize(14);
        $this->SetStyle('U',true);
        foreach ($mappedmarkers as $marker) {
            $links[$marker["Hash"]] = $this->AddLink();
            $this->Write(6, $marker["Name"], $links[$marker["Hash"]]);
            $this->Ln(5);
        }
        $this->SetStyle('U',false);
        $this->SetTextColor(0);
        return $links;
    }

    function MarkerHead($title, $link=null, $author = null, $date = null) {
        $this->Ln(15);
        $this->SetFont('Arial', 'BU', 18);
        $this->Cell(0,8,$title,0,1,"L");
        if (!is_null($link))
            $this->SetLink($link);
        $this->SetFont('Arial', '', 12);
        if (!is_null($author))
            $byl = "By: <i>" . $author . "</i>  ";
        if (!is_null($date))
            $byl .= "on <i>" . $date . "</i>";
        $this->WriteHTML($byl);
        $this->Ln(9);
    }
/*

*/
}
?>