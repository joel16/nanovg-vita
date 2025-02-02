NanoVG-vita
==========

NanoVG is small antialiased vector graphics rendering library for OpenGL. It has lean API modeled after HTML5 canvas API. It is aimed to be a practical and fun toolset for building scalable user interfaces and visualizations.

<p align="center">
<img src="https://i.imgur.com/ItWDrqt.png" alt="nanovg-vita screenshot" width="640" height="362"/>
</p>

## Building
This project is dependent on PVR_PSP2. The required modules (libgpu_es4_ext.suprx, libIMGEGL.suprx, libGLESv2.suprx and libpvrPSP2_WSEGL.suprx) will need to be inside of a folder named "data" that lies in same directory as the CMakeLists.txt. You will also need the following stubs libgpu_es4_ext_stub_weak, libGLESv2_stub_weak and libIMGEGL_stub_weak in your build env or inside the libs directory. Then build using the following command:
```
mkdir build && cd build
cmake .. && make
```


## License
The library is licensed under [zlib license](LICENSE.txt)
Fonts used in examples:
- Roboto licensed under [Apache license](http://www.apache.org/licenses/LICENSE-2.0)
- Entypo licensed under CC BY-SA 4.0.
- Noto Emoji licensed under [SIL Open Font License, Version 1.1](http://scripts.sil.org/cms/scripts/page.php?site_id=nrsi&id=OFL)

## Links
Uses [stb_truetype](http://nothings.org) (or, optionally, [freetype](http://freetype.org)) for font rendering.
Uses [stb_image](http://nothings.org) for image loading.

## Credits
- GrapheneCt and contributors of PVR_PSP2 https://github.com/GrapheneCt/PVR_PSP2
- vitasdk
