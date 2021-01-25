# AtomicParsley

![GitHub Workflow Status](https://img.shields.io/github/workflow/status/wez/atomicparsley/CI)

AtomicParsley is a lightweight command line program for reading, parsing and
setting metadata into MPEG-4 files, in particular, iTunes-style metadata.

## Installation

### macOS

* Navigate to the [latest release](https://github.com/wez/atomicparsley/releases/latest)
* Download the `AtomicParsleyMacOS.zip` file and extract `AtomicParsley`

### Windows

* Navigate to the [latest release](https://github.com/wez/atomicparsley/releases/latest)
* Download the `AtomicParsleyWindows.zip` file and extract `AtomicParsley.exe`

### Linux (x86-64)

* Navigate to the [latest release](https://github.com/wez/atomicparsley/releases/latest)
* Download the `AtomicParsleyLinux.zip` file and extract `AtomicParsley`

### Alpine Linux (x86-64 musl libc)

* Navigate to the [latest release](https://github.com/wez/atomicparsley/releases/latest)
* Download the `AtomicParsleyAlpine.zip` file and extract `AtomicParsley`

### Building from Source

If you are building from source you will need `cmake` and `make`.
On Windows systems you'll need Visual Studio or MingW.

```
cmake .
cmake --build . --config Release
```

will generate an `AtomicParsley` executable.

### Dependencies:

zlib  - used to compress ID3 frames & expand already compressed frames
        available from http://www.zlib.net


## A note on maintenance!

> I made some fixes to the original project on sourceforge back in in 2009 and
> became the de-facto fork of AtomicParsley as a result.  However, I haven't
> used this tool myself in many years and have acted in a very loose guiding
> role since then.
>
> In 2020 Bitbucket decided to cease hosting Mercurial based repositories
> which meant that I had to move it in order to keep it alive, so you'll
> see a flurry of recent activity.
>
> I'll consider merging pull requests if they are easy to review, but because
> I don't use this tool myself I have no way to verify complex changes.
> If you'd like to make such a change, please consider contributing some
> kind of basic automated test with a corresponding small test file.
>
> This repo has GitHub Actions enabled for the three major platforms
> so bootstrapping some test coverage is feasible.
>
> You are welcome to report issues using the issue tracker, but I (@wez)
> am unlikely to act upon them.

