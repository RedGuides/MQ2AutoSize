---
tags:
  - plugin
resource_link: "https://www.redguides.com/community/resources/mq2autosize.100/"
support_link: "https://www.redguides.com/community/threads/mq2autosize.66805/"
repository: "https://github.com/RedGuides/MQ2AutoSize"
config: "MQ2AutoSize.ini"
authors: "dencelle, ieatacid, pms, eqmule, Psycotic"
tagline: "Shrinks anyone in range down to minimum allowed size"
---

# MQ2AutoSize
<!--desc-start-->
Shrinks anyone, including yourself, down to minimum allowed size.
Other players will revert back to normal when they move out of range. These effects are CLIENT SIDE ONLY!
<!--desc-end-->

## Commands

<a href="cmd-autosize/">
{% 
  include-markdown "projects/mq2autosize/cmd-autosize.md" 
  start="<!--cmd-syntax-start-->" 
  end="<!--cmd-syntax-end-->" 
%}
</a>
:    {% include-markdown "projects/mq2autosize/cmd-autosize.md" 
        start="<!--cmd-desc-start-->" 
        end="<!--cmd-desc-end-->" 
        trailing-newlines=false 
     %} {{ readMore('projects/mq2autosize/cmd-autosize.md') }}

## Settings

Example MQ2AutoSize.ini,

```ini

# "Range" is the radius in feet that will be shrunk
# "Size" is the size they will be shrunk down to
# "AutoSize"  on = will shrink everyone within "Range"
# "AutoSizeAll"  on = will shrink everyone in zone upon zone-in
# "ShrinkPets" = Toggle to shrink pets along with PC's

[Config]
Range=50
Size=2
PetSize=1
AutoSize=on
AutoSizeAll=off
ShrinkPets=on
AutoSave=off
ResizePC=on
ResizeNPC=off
ResizePets=off
ResizeMercs=off
ResizeAll=off
ResizeMounts=off
ResizeCorpse=off
ResizeSelf=on
SizeByRange=off
SizeDefault=3
SizePC=4
SizeNPC=3
SizePets=2
SizeMercs=1
SizeTarget=4
SizeMounts=2
SizeCorpse=1
SizeSelf=4
```

"Resize" options are for resizing the spawn type, and can be either ON or OFF.

"Size" options are for the size you want the spawn type, and can be from 1 to 250. These are all client-side only changes.
