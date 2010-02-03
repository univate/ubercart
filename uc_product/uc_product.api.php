<?php
// $Id: uc_product.api.php,v 1.1 2010-02-03 14:19:07 islandusurper Exp $

/**
 * @file
 * Hooks provided by the Product module.
 */

/**
 * @addtogroup hooks
 * @{
 */

/**
 * Perform actions on product classes.
 *
 * @param $type
 *   The node type of the product class.
 * @param $op
 *   The action being performed on the product class:
 *   - "insert": A new node type is created, or an existing node type is being
 *       converted into a product type.
 *   - "update": A product class has been updated.
 *   - "delete": A product class has been deleted. Modules that have attached
 *       additional information to the node type because it is a product type
 *       should delete this information.
 */
function hook_product_class($type, $op) {
  switch ($op) {
    case 'delete':
      db_query("DELETE FROM {uc_class_attributes} WHERE pcid = '%s'", $type);
      db_query("DELETE FROM {uc_class_attribute_options} WHERE pcid = '%s'", $type);
    break;
  }
}

/**
 * Return a structured array representing the given product's description.
 *
 * Modules that add data to cart items when they are selected should display it
 * with this hook. The return values from each implementation will be
 * sent through to hook_product_description_alter() implementations and then
 * all descriptions are rendered using drupal_render().
 *
 * @param $product
 *   Product. Usually one of the values of the array returned by
 *   uc_cart_get_contents().
 * @return
 *   A structured array that can be fed into drupal_render().
 */
function hook_product_description($product) {
  $description = array(
    'attributes' => array(
      '#product' => array(
        '#type' => 'value',
        '#value' => $product,
      ),
      '#theme' => 'uc_product_attributes',
      '#weight' => 1,
    ),
  );

  $desc =& $description['attributes'];

  // Cart version of the product has numeric attribute => option values so we
  // need to retrieve the right ones
  $weight = 0;
  if (empty($product->order_id)) {
    foreach (_uc_cart_product_get_options($product) as $option) {
      if (!isset($desc[$option['aid']])) {
        $desc[$option['aid']]['#attribute_name'] = $option['attribute'];
        $desc[$option['aid']]['#options'] = array($option['name']);
      }
      else {
        $desc[$option['aid']]['#options'][] = $option['name'];
      }
      $desc[$option['aid']]['#weight'] = $weight++;
    }
  }
  else {
    foreach ((array)$product->data['attributes'] as $attribute => $option) {
      $desc[] = array(
        '#attribute_name' => $attribute,
        '#options' => $option,
        '#weight' => $weight++,
      );
    }
  }

  return $description;
}

/**
 * Alters the given product description.
 *
 * @param $description
 *   Description array reference.
 * @param $product
 *   The product being described.
 */
function hook_product_description_alter(&$description, $product) {
  $description['attributes']['#weight'] = 2;
}

/**
 * List node types which should be considered products.
 *
 * Trusts the duck philosophy of object identification: if it walks like a duck,
 * quacks like a duck, and has feathers like a duck, it's probably a duck.
 * Products are nodes with prices, SKUs, and everything else Übercart expects
 * them to have.
 *
 * @return
 *   Array of node type ids.
 */
function hook_product_types() {
  return array('product_kit');
}

/**
 * Notify core of any SKUs your module adds to a given node.
 *
 * NOTE: DO NOT map the array keys, as the possibility for numeric SKUs exists, and
 * this will conflict with the behavior of module_invoke_all(), specifically
 * array_merge_recursive().
 *
 * Code lifted from uc_attribute.module.
 */
function hook_uc_product_models($node) {
  $models = array();

  // Get all the SKUs for all the attributes on this node.
  $adjustments = db_query("SELECT model FROM {uc_product_adjustments} WHERE nid = %d", $node->nid);
  while ($adjustment = db_fetch_object($adjustments)) {
    if (!in_array($adjustment->model, $models)) {
      $models[] = $adjustment->model;
    }
  }

  return $models;
}

/**
 * @} End of "addtogroup hooks".
 */

