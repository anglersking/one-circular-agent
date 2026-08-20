# Third-Party Notices

## Espressif Brookesia emotion assets

The `assets/emotion_*_284_126.aaf` files are derived from the emotion assets
used by the Espressif Brookesia speaker product:

https://github.com/espressif/esp-brookesia/tree/release/v0.6/products/speaker

The firmware decodes these assets with the `espressif2022/image_player`
component and maps their grayscale masks to the DeepSeek blue palette. The
angry animation follows the upstream white-to-red timing, with the white phase
replaced by blue. Espressif Brookesia and `image_player` are distributed under
the Apache License 2.0.

https://www.apache.org/licenses/LICENSE-2.0

## ElectronBot reference asset

`assets/electronbot_blue_idle.gif` is a derived screen asset based on the
ElectronBot Standalone Lottie expression project:

https://github.com/maker-community/ElectronBot.Standalone

Copyright (c) 2024 创客社区（Hacker space）

The upstream project is distributed under the MIT License. The derived asset
keeps the original animation timing and geometry, with the eye color changed
to blue for this feature test.

```text
MIT License

Copyright (c) 2024 创客社区（Hacker space）

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
```
