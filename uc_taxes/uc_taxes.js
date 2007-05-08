// -*- js-var: set_line_item, base_path, li_titles, li_values; -*-
// $Id: uc_taxes.js,v 1.4 2007-05-08 21:13:24 rszrama Exp $

var pane = '';
if ($("input[@name*=delivery_]").length){
  pane = 'delivery'
}
else if($("input[@name*=billing_]").length){
  pane = 'billing'
}

$(document).ready(function(){
  getTax();
  $("select[@name*=" + pane + "_zone], input[@name*=" + pane + "_postal_code], select[@name*=address_select]").change(getTax);
});

function getTax(){
  var order_size = 5;
  var line_item = '';
  var key;
  var i = 0;
  for (key in li_titles){
    if (key != 'subtotal'){
      line_item = 'i:' + i + ';a:2:{s:4:"type";s:'+ key.length + ':"'+ key + '";s:6:"amount";d:' + li_values[key] + ';}';
      i++;
    }
  }
  line_item = 's:10:"line_items";a:' + i + ':{' + line_item + '}';
  var order = 'O:8:"stdClass":' + order_size + ':{s:8:"products";' + encodeURIComponent($("[@name=cart_contents]").val())
    + 's:13:"delivery_zone";i:' + $("select[@name*=" + pane + "_zone] option:selected").val()
    + ';s:20:"delivery_postal_code";s:' + $("input[@name*=" + pane + "_postal_code]").val().length +':"' + encodeURIComponent($("input[@name*=" + pane + "_postal_code]").val())
  + '";s:16:"delivery_country":i:840;' + line_item + '}';
  $.ajax({
    type: "POST",
    url: base_path + "?q=taxes/calculate",
    data: 'order=' + order,
    dataType: "json",
    success: function(taxes){
    //if (taxes.constructor == Array){
      var i;
      for (i in taxes){
        set_line_item("tax_" + taxes[i].id, taxes[i].name, taxes[i].amount, 9);
      }
    //}
    }
  });
}
