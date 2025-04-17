<?php

class Recipe {
  public $name;
  public $path;
  public $status;

  public $info = [];

  // Config
  // Content
  // Styles
  public $components = [];

  public function get_components() {
    if (empty($this->components)) {
      // idk
    }

    return $this->components;
  }

  public function get_info() {}
  public function add_dependency($module) {
    if (!in_array($module, $this->info['dependencies'])) {
      $this->info['dependencies'][] = $module;
    }
  }
}
