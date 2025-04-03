<?php

function recipe_builder_build_form($form, &$form_state, $recipe = NULL) {
  global $config_directories;

  backdrop_add_css(backdrop_get_path('module', 'recipe_builder') . '/css/recipe_builder.admin.css');

  // Load the recipe from the tempstore if build mode is active.
  if (state_get('recipe_builder_build_mode', FALSE)) {
    if (empty($recipe)) {
      $recipe = tempstore_get('recipe_builder_build_mode', 'recipe');
    }
  }

//  dpm($recipe);

  $form = array(
    '#recipe' => isset($recipe) ? $recipe : new Recipe(),
  );

  $form['info'] = [
    '#type' => 'fieldset',
    '#title' => t('Recipe Information'),
  ];

  $form['info']['name'] = [
    '#title' => t('Name'),
    '#type' => 'textfield',
    '#default_value' => !empty($recipe->info['name']) ? $recipe->info['name'] : '',
    '#description' => t('Example: Image gallery') . ' (' . t('Do not begin name with numbers.') . ')',
    '#required' => TRUE,
  ];
  $form['info']['module_name'] = array(
    '#type' => 'machine_name',
    '#title' => t('Machine-readable name'),
    '#description' => t('Example: image_gallery') . '<br/>' . t('May only contain lowercase letters, numbers and underscores. <strong>Try to avoid conflicts with the names of existing Backdrop projects.</strong>'),
    '#required' => TRUE,
    '#default_value' => $recipe->name,
    '#machine_name' => array(
      'exists' => 'module_exists',
      'source' => array('info', 'name'),
    ),
  );

  // If recreating this recipe, disable machine name field to ensure the
  // machine name cannot be changed, unless user role has granted permission to
  // edit machine name of disabled recipes.
  if (isset($recipe) && ($recipe->status || !user_access('rename recipes'))) {
    $form['info']['module_name']['#value'] = $recipe->name;
    $form['info']['module_name']['#disabled'] = TRUE;
  }

  $form['info']['description'] = array(
    '#title' => t('Description'),
    '#description' => t('Provide a short description of what users should expect when they enable your recipe.'),
    '#type' => 'textfield',
    '#default_value' => !empty($recipe->info['description']) ? $recipe->info['description'] : '',
  );

  $form['info']['package'] = array(
    '#title' => t('Package'),
    '#description' => t('Organize your Recipes in groups. Use "Recipes" if you are using the Recipes module.'),
    '#type' => 'textfield',
    '#autocomplete_path' => 'recipe_builder/autocomplete/packages',
    '#default_value' => !empty($recipe->info['package']) ? $recipe->info['package'] : 'Recipes',
  );

  $form['info']['path'] = [
    '#type' => 'textfield',
    '#title' => t('Path'),
    '#default_value' => !empty($recipe->path) ? $recipe->path : '',
    '#description' => t('File path for the Recipe.  For Example: modules/recipes/[module_name] or /tmp[module_name].'),
  ];

  // @TODO: Add support for more info setting types.
//  $form['info']['tags'] = [
//    '#type' => 'textfield',
//    '#title' => t('Tags'),
//    '#default_value' => tempstore_get('recipe_builder_build_mode', 'tags'),
//    '#description' => t('A comma seperated list of tags for searching on the '),
//  ];;

  _recipe_builder_build_form_components($form, $form_state);

  $block_starting = FALSE;
  $private_files = config_get('system.core', 'file_private_path');
  if (empty($private_files)) {
    backdrop_set_message('The private file path is not set.', 'warning');
    $block_starting = TRUE;
  }
  if (!is_writable($private_files)) {
    backdrop_set_message('The private files directory is not writable.', 'warning');
    $block_starting = TRUE;
  }

  $form['actions'] = [
    '#type' => 'fieldset',
    '#title' => t('Actions'),
  ];

  if (isset($config_directories['recipe_builder']) && !empty($config_directories['recipe_builder'])) {
    if (state_get('recipe_builder_build_mode', FALSE)) {
      $form['actions']['cancel_build_mode'] = [
        '#type' => 'submit',
        '#value' => t('Cancel Build Mode'),
      ];
    }
    else {
      $form['actions']['start_build_mode'] = [
        '#type' => 'submit',
        '#value' => t('Start Build Mode'),
        '#disabled' => $block_starting,
      ];
    }
  }

  $form['actions']['save'] = [
    '#type' => 'submit',
    '#value' => t('Save'),
  ];

//  dpm($form);
  return $form;
}

function _recipe_builder_build_form_components(&$form, &$form_state) {
  $form['components'] = [
    '#type' => 'fieldset',
    '#title' => t('Components'),
  ];

  $recipe = $form['#recipe'];

  // @TODO: Add a way to add an existing component to the list.
//  $form['components']['add_component'] = [
//    '#type' => 'textfield',
//    '#title' => t('Add component'),
//    '#description' => t('Add a component to the recipe.'),
//  ];

  $prefix_groups = config_get_prefix_groups('active');

  // Get existing components from the recipe.
  $components = $recipe->get_components();

  // Get componenets from build mode.
  if (state_get('recipe_builder_build_mode', FALSE)) {
    // Configuration.
    $config_status = _recipe_builder_get_config_status();
//    dpm($config_status);
    foreach ($config_status as $config_name => $status) {
      if ($status !== 'default') {
        $components['configuration'][$config_name] = $config_name;
      }
    }

    // Dependencies.
    $active_modules = module_list();
    $recipe_builder_modules = tempstore_get('recipe_builder_build_mode', 'module_list');
    foreach ($active_modules as $module_name) {
      if (!in_array($module_name, $recipe_builder_modules)) {
        $components['dependencies'][$module_name] = $module_name;
      }
    }

    // Get the prefix groups from the recipe_builder config.
    $prefix_groups = backdrop_array_merge_deep($prefix_groups, config_get_prefix_groups('recipe_builder'));
  }

  // @TODO: Add hook_component_info() so that modules can add new component types.

  if (!empty($components['configuration'])) {
    $form['components']['configuration'] = [
      '#type' => 'fieldset',
      '#title' => t('Configuration'),
      '#tree' => TRUE,
    ];

    foreach ($components['configuration'] as $file) {
      $form['components']['configuration'][$file] = [
        '#type' => 'container',
        '#attributes'=> [
          'class' => [
            'status-' . $config_status[$file],
          ],
        ],
      ];
      $form['components']['configuration'][$file][$file] = [
        '#type' => 'checkbox',
        '#title' => $file,
        '#parents' => ['configuration', $file],
      ];

      // If the config is already in the recipe, check it by default.
      if (!empty($recipe->components['configuration']) && in_array($file, $recipe->components['configuration'])) {
        $form['components']['configuration'][$file][$file]['checked'] = TRUE;
      }
      foreach ($components['dependencies'] as $module) {
        if (_recipe_builder_check_default_config($module, $file)) {
          $form['components']['configuration'][$file][$file]['#description'] = t('Added by @module.', ['@module' => $module]);
          break;
        }
      }

      // Provide a diff if the file was modified.
      if ($config_status[$file] === 'changed') {
        if (empty($formatter)) {
          $formatter = new BackdropDiffFormatter();
          $active_storage = config_get_config_storage('active');
          $recipe_builder_storage = config_get_config_storage('recipe_builder');
        }

        $active_data = $active_storage->read($file);
        $active_data = explode("\n", backdrop_json_encode($active_data, TRUE));

        $recipe_builder_data = $recipe_builder_storage->read($file);
        $recipe_builder_data = explode("\n", backdrop_json_encode($recipe_builder_data, TRUE));

        $diff = new Diff($recipe_builder_data, $active_data);

        $form['components']['configuration'][$file]['preview'] = [
          '#type' => 'fieldset',
          '#title' => t('Preview'),
          '#collapsed' => TRUE,
          '#collapsible' => TRUE,
          'diff' => [
            'table' => [
              '#theme' => 'table',
              '#header' => array(
                array(
                  'data' => 'Old', 'colspan' => '2',
                  'class' => array('config-old'),
                ),
                array(
                  'data' => 'New', 'colspan' => '2',
                  'class' => array('config-new'),
                ),
              ),
              '#attributes' => array('class' => array('diff-table')),
              '#rows' => $formatter->format($diff),
              '#sticky' => FALSE,
            ],
          ]
        ];
      }
    }
  }

  if (!empty($components['dependencies'])) {
    $form['components']['dependencies'] = [
      '#type' => 'fieldset',
      '#tree' => TRUE,
      '#title' => t('Dependencies'),
      '#description' => t('The selected modules will be added as dependencies of the Recipe.'),
    ];
    foreach ($components['dependencies'] as $module_name) {
      $module_info = system_get_info('module', $module_name);
      $form['components']['dependencies'][$module_name] = [
        '#type' => 'checkbox',
        '#title' => $module_info['name'],
      ];
      if (!empty($recipe->info['dependencies']) && in_array($file, $recipe->info['dependencies'])) {
        $form['components']['dependencies'][$module_name]['#checked'] = TRUE;
      }
    }
  }
}

function recipe_builder_build_form_validate($form, &$form_state) {
//  dpm($form_state);

  if (isset($form_state['input']['op'])) {
    if ($form_state['input']['op'] === $form_state['values']['start_build_mode']) {
      // Make sure that recipe_builder storage exists, is writable, and is empty.
    }
    if ($form_state['input']['op'] === $form_state['values']['save']) {
      $path = !empty($form_state['values']['path']) ? $form_state['values']['path'] : 'modules/' . $form_state['values']['module_name'];

      $writable = file_prepare_directory($path, FILE_CREATE_DIRECTORY);
      if (!$writable) {
        form_set_error('path', t('The module directory could not be created or is not writable.'));
      }
    }
  }
}

function recipe_builder_build_form_submit($form, &$form_state) {
  if (isset($form_state['input']['op'])) {

    // Save module info into tempstore.
    if ($form_state['input']['op'] === $form_state['values']['start_build_mode']
      || $form_state['input']['op'] === $form_state['values']['save']) {
      $recipe = $form['#recipe'];

      $existing_components = $recipe->get_components();

      $recipe->name = $form_state['values']['module_name'];
      $recipe->path = !empty($form_state['values']['path']) ? $form_state['values']['path'] : 'modules/' . $form_state['values']['module_name'];
      $recipe->info['name'] = $form_state['values']['name'];
      $recipe->info['description'] = $form_state['values']['description'];
      $recipe->info['package'] = $form_state['values']['package'];
    }

    // Start build mode.
    if ($form_state['input']['op'] === $form_state['values']['start_build_mode']) {
      $active_storage =  config_get_config_storage('active');
      $active_files = $active_storage->listAll();

      // Save all the active config into the recipe_builder config storage.
      foreach ($active_files as $file_name) {
        $active_config = config($file_name, 'active');
        $recipe_builder_config = config($file_name, 'recipe_builder');

        $recipe_builder_config->setData($active_config->getData());
        $recipe_builder_config->save();
      }

      $expire = strtotime('+100 days');
      tempstore_set('recipe_builder_build_mode', 'recipe', $recipe, $expire);
      tempstore_set('recipe_builder_build_mode', 'module_list', module_list(), $expire);

      // Start build mode.
      state_set('recipe_builder_build_mode', TRUE);
    }

    if ($form_state['input']['op'] === $form_state['values']['save']) {
      // Create info file.
      $info_defaults = [
        'type' => 'module',
        'backdrop' => '1.x',
        'recipe' => 'true',
      ];

      // Process dependencies.
      foreach ($form_state['input']['dependencies'] as $module_name => $value) {
        if ($value) {
          $recipe->info['dependencies'][] = $module_name;
        }
      }

      $info_output = recipe_builder_export_info(array_merge($info_defaults, $recipe->info));

      $info_path = $recipe->path . '/' . $recipe->name . '.info';
      file_put_contents($info_path, $info_output);

      // Create a .module file.
      if (!file_exists($recipe->path . '/' . $recipe->name . '.module')) {
        file_put_contents($recipe->path . '/' . $recipe->name . '.module', '');
      }

      // Process config files.
      if (!empty($form_state['input']['configuration'])) {
        $status = _recipe_builder_get_config_status();

        $recipe_storage = new ConfigFileStorage($recipe->path . '/config');
        if (!$recipe_storage->isInitialized()) {
          $recipe_storage->initializeStorage();
        }
        foreach ($form_state['input']['configuration'] as $config_name => $value) {
          if ($value) {
            switch ($status[$config_name]) {
              case 'new':
              case 'default':
              case 'changed':
                $config = new Config($config_name, $recipe_storage);
                $active_config = config($config_name);
                $config->setData($active_config->getData());
                $config->save();
                break;

              case 'deleted':
                $config = new Config($config_name, $recipe_storage);
                $config->delete();
                break;
            }
          }
        }
      }
    }

    // Clean up recipe builder data.
    if ($form_state['input']['op'] === $form_state['values']['cancel_build_mode']
      || $form_state['input']['op'] === $form_state['values']['save']) {
      state_set('recipe_builder_build_mode', FALSE);

      tempstore_clear_all('recipe_builder_build_mode');

      $storage = config_get_config_storage('recipe_builder');
      $storage->deleteAll();
    }

  }

//  dpm(__FUNCTION__);
//  dpm($form_state['input']['op']);
//  dpm($form, '$form');
//  dpm($form_state, '$form_state');
}

function _recipe_builder_get_config_status($config = NULL) {
  static $statuses = [];

  if (empty($statuses)) {
    $active_storage = config_get_config_storage('active');
    $active_config = $active_storage->listAll();

    $recipe_builder_storage = config_get_config_storage('recipe_builder');
    $recipe_builder_config = $recipe_builder_storage->listAll();

    $all_config = array_merge($active_config, $recipe_builder_config);

    $formatter = new BackdropDiffFormatter();

    foreach ($all_config as $config_name) {
      if (!in_array($config_name, $recipe_builder_config)) {
        $statuses[$config_name] = 'new';
        continue;
      }
      if (!in_array($config_name, $active_config)) {
        $statuses[$config_name] = 'deleted';
        continue;
      }

      $active_data = $active_storage->read($config_name);
      $active_data = explode("\n", backdrop_json_encode($active_data, TRUE));

      $recipe_builder_data = $recipe_builder_storage->read($config_name);
      $recipe_builder_data = explode("\n", backdrop_json_encode($recipe_builder_data, TRUE));

      $diff = new Diff($active_data, $recipe_builder_data);
      if (!$diff->isEmpty()) {
        $statuses[$config_name] = 'changed';
        continue;
      }

      $statuses[$config_name] = 'default';
    }
  }

  if ($config !== NULL) {
    return $statuses[$config_name];
  }
  return $statuses;
}
