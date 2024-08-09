# MQ2AutoSize

## Usage:

This plugin will automatically resize configured spawn groups to the specified size. You can configure it to only resize within a specific range and then resize back to normal when your distance moves out of that range. Current default range is set to 50 and may be changed via UI panel, INI or cmd line.

NOTE: These effects are client side only.

## Toggles (you may also append on or off to set the value):

* /autosize autosave   - Automatically save settings to INI file when an option is toggled or size is set
* /autosize            - Toggles AutoSize functionality (on/off)
* /autosize dist       - Toggles distance-based vs Zonewide (range 1000)
* /autosize pc         - Toggles AutoSize PC spawn types (on/off)
* /autosize npc        - Toggles AutoSize NPC spawn types (on/off)
* /autosize pets       - Toggles AutoSize pet spawn types (on/off)
* /autosize mercs      - Toggles AutoSize mercenary spawn types (on/off)
* /autosize mounts     - Toggles AutoSize mounted player spawn types (on/off)
* /autosize corpse     - Toggles AutoSize corpse spawn types (on/off)
* /autosize everything - Toggles AutoSize all spawn types (on/off)
* /autosize self       - Toggles AutoSize for your character (on/off)

## Size configuration (valid sizes 1 to 250)
* /autosize range #      - Sets range for distance-based AutoSize
* /autosize sizeself #   - Sets size for your character
* /autosize sizepc #     - Sets size for PC spawn types
* /autosize sizenpc #    - Sets size for NPC spawn types
* /autosize sizepets #   - Sets size for pet spawn types
* /autosize sizemercs #  - Sets size for mercenary spawn types
* /autosize sizemounts # - Sets size for mounted player spawn types
* /autosize sizecorpse # - Sets size for corpse spawn types

## Other Commands:
* /autosize status - Display current plugin settings to MQ Console and or chatwnd if loaded
* /autosize [gui | ui | show] - Display the ImGui panel under /mqsettings
* /autosize help   - Display command syntax to MQ Console and or chatwnd if loaded
* /autosize save   - Save settings to INI file (auto on plugin unload)
* /autosize load   - Load settings from INI file (auto on plugin load)
