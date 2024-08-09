8/09/2024
--
Added
- command to open UI: /autosize [gui | ui | show]

7/26/2024
--
Added
- UI and command to toggle the AutoSize functionality (`/autosize [on|off]`)
- new TLO member: Enabled (`${AutoSize.Enabled}` or `mq.TLO.AutoSize.Enabled()`)

Updated
- Wording for `/autosize` and `/autosize dist [on|off]` to better explain how to use them

7/11/2024
--
Added
- MQ Settings panel (/mqsettings -> plugin -> AutoSize)
- AutoSize TLO (check wiki for details)
- Introduced Synchronization ability - only available in the UI
- Enhanced options to accept instruction such as on and off, not just be a toggle. This was done to enable keeping toons in sync by setting values instead of randomly toggling.

Updated / Fixed
- Zonewide now uses 1000 range
- /autosize range # now works
- Reduced organic growth of code and made all toggle options use the ToggleOption function
- Fixed and altered how "Everything" works. Now it will enable the options, which work as intended.

Deprecated
- OptZone since it never worked as expected, replaced with Range of 1000 which is about the same as the EQ visible max clipping plane
- ResizeAll configuration, while Everything is still able to be enabled
- DefaultSize configuration and command (/autosize size), while Everything is still able to be enabled
- SizeDefault since having a default setting to resize something to, which wasn't opted in for resizing, doesn't actually make any sense. Outputs have persisted to ensure no script that scrapes the line breaks.
- SizeTarget (and /autosize target) since it would only resize for one frame if the type was managed by an Option for resizing
- OptByRange and SizeByRange, since the plugin now only uses range to resize


Under the hood
- Changed the majority of floats to integer as there was no reason for using all the floats
- Updated pCharSpawn to pLocalPlayer references
