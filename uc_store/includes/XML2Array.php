<?php
// $Id: XML2Array.php,v 1.3 2007-10-02 17:43:48 rszrama Exp $

/* 
   By: bbellwfu@gmail.com from PHP.net comments for xml_parse!

   Usage
   Grab some XML data, either from a file, URL, etc. however you want. Assume storage in $strYourXML;

   $objXML = new xml2Array();
   $arrOutput = $objXML->parse($strYourXML);
   print_r($arrOutput); //print it out, or do whatever!
*/

class xml2Array {

  var $arrOutput = array();
  var $resParser;
  var $strXmlData;

  function parse($strInputXML) {
    $this->resParser = xml_parser_create ();
    xml_set_object($this->resParser,$this);
    xml_set_element_handler($this->resParser, "tagOpen", "tagClosed");
       
    xml_set_character_data_handler($this->resParser, "tagData");
    
    $this->strXmlData = xml_parse($this->resParser,$strInputXML );
    if(!$this->strXmlData) {
      die(sprintf("XML error: %s at line %d",
                  xml_error_string(xml_get_error_code($this->resParser)),
                  xml_get_current_line_number($this->resParser)));
    }

    xml_parser_free($this->resParser);

    return $this->arrOutput;
  }

  function tagOpen($parser, $name, $attrs) {
    $tag=array("name"=>$name,"attrs"=>$attrs);
    array_push($this->arrOutput,$tag);
  }

  function tagData($parser, $tagData) {
    if(trim($tagData) !== '') {
      if(isset($this->arrOutput[count($this->arrOutput)-1]['tagData'])) {
        $this->arrOutput[count($this->arrOutput)-1]['tagData'] .= $tagData;
      }
      else {
        $this->arrOutput[count($this->arrOutput)-1]['tagData'] = $tagData;
      }
    }
  }

  function tagClosed($parser, $name) {
    $this->arrOutput[count($this->arrOutput)-2]['children'][] = $this->arrOutput[count($this->arrOutput)-1];
    array_pop($this->arrOutput);
  }
}
