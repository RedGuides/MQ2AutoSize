---
tags:
  - command
---

# /autosize

## Syntax

<!--cmd-syntax-start-->
```eqcommand
/autosize [on|off] | [option] [#]
```
<!--cmd-syntax-end-->

## Description

<!--cmd-desc-start-->
Handles autosize settings and status
<!--cmd-desc-end-->

## Options

| Option | Description |
|--------|-------------|
| `[on|off]` | Turns sizing on and off |
| `dist` | Toggles distance-based AutoSize on/off |
| `pc` | Toggles AutoSize PC spawn types |
| `npc` | Toggles AutoSize NPC spawn types |
| `pets` | Toggles AutoSize pet spawn types |
| `mercs` | Toggles AutoSize mercenary spawn types |
| `mounts` | Toggles AutoSize mounted player spawn types |
| `corpse` | Toggles AutoSize corpse spawn types |
| `target` | Resizes your target to sizetarget size |
| `everything` | Toggles AutoSize all spawn types |
| `self` | Toggles AutoSize for your character |
| `range #` | Sets range for distance-based AutoSize. Example: `/autosize range 50` |
| `size #` | Sets default size for "everything"<br><br>(Valid sizes 1 to 250) |
| `sizepc #` | Sets size for PC spawn types |
| `sizenpc #` | Sets size for NPC spawn types |
| `sizepets #` | Sets size for pet spawn types |
| `sizetarget #` | Sets size for target parameter |
| `sizemercs #` | Sets size for mercenary spawn types |
| `sizemounts #` | Sets size for mounted player spawn types |
| `sizecorpse #` | Sets size for corpse spawn types |
| `sizeself #` | Sets size for your character |
| `status` | Display current plugin settings |
| `help` | Displays command syntax |
| `save` | Save settings to INI file (auto on plugin unload) |
| `load` | Load settings from INI file (auto on plugin load) |
| `autosave` | Automatically save settings to INI file when an option is toggled or size is set |

## Examples

- Sets size of mercs to 2  
  ```eqcommand
  /autosize sizemercs 2
  ```
- Toggles AutoSize for mercs  
  ```eqcommand
  /autosize mercs
  ```
- Sets AutoSize range to 50  
  ```eqcommand
  /autosize range 50
  ```
