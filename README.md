A mod by [Serialbocks](https://twitter.com/Serialbocks)

Supersonic Mario puts Mario from Super Mario 64 into Rocket League. This page details how to install and play the mod. You will be required to provide a valid Super Mario 64 U.S. version ROM to play. Supersonic Mario contains no Nintendo assets, nor do I claim ownership to any Nintendo assets.

[Download](https://github.com/Serialbocks/SupersonicMarioPlugin/releases/tag/v1.1.1) and Installation

The installer will guide you through building the mod as well as installing MSYS2 and Bakkesmod, which are both required.

1.  [Download](https://github.com/Serialbocks/SupersonicMarioPlugin/releases/tag/v1.1.1) and run the Supersonic Mario installer. The wizard will guide you through each step.
2.  Run bakkesmod and Rocket League together. Make sure both Rocket League and Bakkesmod are up to date.
3.  At the Rocket League main menu, press F2 to bring up the bakkesmod menu. (If you're asked about bindings, any option is fine). Navigate to the Plugins tab, and then "Open Plugin Manager". Check the box next to Supersonic Mario. Press esc to close this window.
4.  Change your in-game FPS to one divisible by 30 (i.e. 60, 120, 240, etc). Other FPS values cause the mario engine, which was designed to only run at 30 FPS, to run at a different speed than normal. Uncapped FPS won't work.
5.  Press F3 to open the Supersonic Mario menu. You can now play the mod! Note in order to host a game, you will need to forward two ports (7777 and 7778 TCP/UDP by default) for people to be able to join you. For more information on port forwarding, [click here](https://www.hellotech.com/guide/for/how-to-port-forward). We plan on adding a server browser in the future, and perhaps some dedicated servers!

Configuration

Preferences and game settings (while hosting) can be found in the F3 menu. Mario's volume can be adjusted here, as well as the SM64 ROM location in case it changes.

Button Mappings

Mario's moves are mapped using Rocket League's in-game mapping system. The default button mapping for controllers closely mimics the default button mapping for SM64, but can be changed. Keyboard works, but is a little quirky while running (crawling) backwards. The following are the mappings:

1.  N64 A (Jump): Rocket League Jump
2.  N64 B (Kick/Dive): Rocket League Power Slide
3.  N64 Z (Crouch): Rocket League Drive Backwards

Quirks and Known Bugs

If you encounter a bug (there will certainly be some at launch), please report them on the Github Issues page.

*   After a game ends, the host should click the "Host New Match" button from the F3 menu to play again, otherwise the game may be filled with bots.
*   No Himachi support.
*   While controlling Mario with keyboard, running backwards causes mario to crouch.
*   Uncapped FPS or any FPS setting not divisible by 30 causes the SM64 engine to run at an unintended speed.
