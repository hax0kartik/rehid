# Rehid

HID module rewrite with the aim of easier button remapping and more.

## How To Use

You need to have the latest luma for this to work correctly.

* Put the `0004013000001D02` folder in `luma/titles`

* Create a folder named `rehid` on the root of your sd card and put the `rehid.json` file in the folder (Check the [How-To-Remap-Buttons](https://github.com/hax0kartik/rehid#how-to-remap-buttons) part to create the rehid.json file)

* Enable Title patching from luma menu. (As shown here: https://github.com/LumaTeam/Luma3DS/wiki/Optional-features)

* Enter game and check if remappings are working

## How To Remap Buttons

You first need to create a `rehid.json` file with the remappings you want. For eg:-
```Json
{
    "remappings":[
        {"get":"R", "press":"ZR"},
        {"get":"L", "press":"ZL"}
    ]
}
```
With the above, everytime you press `ZR` key, `R` key would be triggered, 

and everytime you press `ZL` key, `L` key would be triggered.

It is also possible to do custom key combos, i.e.,
```Json
{
    "remappings":[
        {"get":"R", "press":"X+Y"},
        {"get":"L+R", "press":"SELECT"}
    ]
}
```
Now everytime you press `X+Y`, `R` key would be triggered and on pressing `SELECT` button, both `L` and `R` would be triggered.

Possible Keys are:- 
`A`, `B`, `X`, `Y`, `SELECT`, `START`, `ZL`, `ZR`, `L`, `R`, `LEFT`, `RIGHT`, `UP`, `DOWN`, `CRIGHT`(CPAD), `CLEFT`(CPAD), `CUP`(CPAD), `CDOWN`(CPAD)

Copy your `rehid.json` file to the `rehid` folder.

### Per Title Button Remapping

It is possible to have different button remapings for different titles:-

Inside the `rehid` folder, create a folder with the titleid as the folder name.

You can use [this](https://hax0kartik.github.io/3dsdb/) to fidn the titleid for your game.

Copy the `rehid.json` file inside this folder.

## Compilation
Get devkitpro, ctrulib and makerom and then `make -j` to compile.

## Credits

@luigoalma Help, testing and listening to my rants.

Druivensap on my discord server for helping me test out .

Luma3ds devs and contributors
