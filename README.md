<!-- This file is mostly readable as plain text, but it will look better
  -- in a Markdown reader (try Frogmouth).
  -- 
  -- Alternately, go to https://hpjansson.org/chafa/ in a web browser. -->

<p align="center"><a href="https://hpjansson.org/chafa/"><img src="https://raw.githubusercontent.com/hpjansson/chafa/master/docs/chafa-logo.gif" /></a></p>
<p align="center"><a href="https://github.com/hpjansson/chafa/actions/workflows/c-cpp.yml"><img src="https://img.shields.io/github/actions/workflow/status/hpjansson/chafa/c-cpp.yml?style=for-the-badge" alt="Master Build Status" /></a>&emsp;<a href="https://hpjansson.org/chafa/download/"><img src="https://img.shields.io/github/v/release/hpjansson/chafa?style=for-the-badge" alt="Latest Release" /></a>&emsp;<a href="https://matrix.to/#/#chafa:matrix.org"><img src="https://img.shields.io/badge/friendly_chat-%23chafa%3Amatrix.org-blue?style=for-the-badge&logo=matrix" alt="Friendly Chat" /></a></p>
<p align="center"><a href="https://hpjansson.org/chafa/">About</a> • <a href="https://hpjansson.org/chafa/gallery/">Gallery</a> • <a href="https://hpjansson.org/chafa/download/">Packages</a> • <a href="https://hpjansson.org/chafa/development/">Development</a></p>

## About

Chafa is a command-line utility that converts image data, including
animated GIFs, into graphics formats or ANSI/Unicode character art suitable
for display in a terminal. It has broad feature support, allowing it to be
used on devices ranging from historical teleprinters to modern terminal
emulators and everything in between.

The core functionality is provided by a C library with a public,
well-documented API.

Both library and frontend tools are covered by the Lesser GPL license,
version 3 or later (LGPLv3+).

The [official web pages](https://hpjansson.org/chafa/) and [C API
documentation](https://hpjansson.org/chafa/ref/) can be found online. Check
out the [gallery](https://hpjansson.org/chafa/gallery/) for screenshots.

## Installing

Chafa is most likely packaged for your distribution, so if you're not
going to hack on it, you're better off using
[official packages](https://hpjansson.org/chafa/download/). If you want to
build the latest and greatest yourself, read on.

You will need GCC, make, Autoconf, Automake, Libtool and the GLib
development package installed to compile Chafa from its git repository. If
you want to build the command-line tool `chafa` and not just the library,
you will additionally need development packages for:

* FreeType2. Often packaged as `libfreetype6-dev` or `freetype2-devel`.
* libavif (optional). Look for `libavif-dev` or `libavif-devel`.
* libdeflate (optional, speeds up PNG decoding). Look for `libdeflate-dev` or `libdeflate-devel`.
* libheif 1.12 or later (optional). Look for `libheif-dev` or `libheif-devel`.
* libjpeg (optional). Look for `libjpeg-dev`, `libjpeg62-devel` or `libjpeg8-devel`.
* libjxl (optional). Look for `libjxl-dev` or `libjxl-devel`.
* librsvg (optional). Look for `librsvg2-dev` or `librsvg-devel`.
* libtiff (optional). Look for `libtiff5-dev` or `libtiff-devel`.
* libwebp (optional). Look for `libwebp-dev` or `libwebp-devel`.

If you want to build documentation, you will also need gtk-doc.

Start by cloning the repository:

```sh
$ git clone https://github.com/hpjansson/chafa.git
```

Then cd to the toplevel directory and issue the following shell commands:

```sh
$ ./autogen.sh
$ make
$ sudo make install
```

That should do it!

## Color values

`--bg` and `--fg` accept X.Org color names, RGB hex values, and decimal
xterm-256 palette indices from `0` to `255`. Indices use one to three digits,
including any leading zeros:
`--bg=1`, `--bg=01`, and `--bg=001` all select red (`#cc0403`).

Indices `0`–`15` use fixed Kitty system colors, `16`–`231` use the standard
xterm RGB cube, and `232`–`255` use its grayscale ramp. Indices are converted
to RGB independently of the terminal's current palette. Six-digit RGB hex
values accept an optional `#` or `0x` prefix: `--bg=112233`, `--bg='#112233'`,
and `--bg=0x112233` all select the same RGB color.

Use `--force-color-fg=COLORS` and `--force-color-bg=COLORS` to restrict the
foreground and background palettes independently. Lists accept color names,
RGB hex, palette indices, and inclusive index ranges, for example:

```sh
chafa --colors=256 --force-color-fg=16-231 --force-color-bg=232-255 image.png
chafa --colors=16 --force-color-fg=1,2,3 --force-color-bg=0,4 --bg=0 image.png
```

Chafa chooses glyphs using the restricted palettes and maps colors to their
nearest candidates. In the second example, colors matching the explicit
`--bg=0` use the terminal's default background rather than painted black.
An explicit `--fg` works the same way for the default foreground. Defaults
are added only when their matching force option is present.

Every requested color, including an explicit default, must exist in the
effective color mode. Numeric indices select exact terminal slots; names and
hex must exactly match a Kitty/xterm palette RGB in indexed modes. Thus `1`
works in 16-color mode, while the name `red` (`#ff0000`, slot `196`) is
available through the extended palette or truecolor. Mode `240` excludes slots `0`–`15`; `16/8`
allows background slots `0`–`7` and foreground slots `0`–`15`.

Force palettes select symbols when the output format is automatic. Explicit
pixel formats and color modes `none`/`2` are rejected. `--fg-only` cannot be
combined with `--force-color-bg`. Repeating a force option replaces its list;
equal-distance matches use list order. These options do not change the
terminal's palette or default color settings.

## Python bindings

[Erica Ferrua Edwardsdóttir](https://mage.black/) maintains
[excellent Python bindings](https://chafapy.mage.black/) for Chafa. If
Python's your thing, check them out. They are easy to use and come with a
[detailed tutorial](https://chafapy.mage.black/usage/tutorial.html).

## JavaScript bindings

[Héctor Molinero Fernández](https://hector.molinero.dev/) maintains
[wonderful JavaScript bindings](https://github.com/hectorm/chafa-wasm)
built around his WebAssembly port. These are available from NPM and can
be used in Node.js, web browsers, and more.
