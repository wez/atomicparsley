# AtomicParsley

![Maintenance](https://img.shields.io/maintenance/no/2012)

**Maintenance Note:** This repo is not actively maintained by @wez.

A note from @wez:

> I made some fixes in 2009 and became the de-facto fork of AtomicParsley as a
> result.  However, I haven't used this tool myself in many years and have
> acted in a very loose guiding role since then.
>
> In 2020 Bitbucket decided to cease hosting Mercurial based repositories
> which meant that I had to move it in order to keep it alive, so you'll
> see a flurry of recent activity.
>
> I'll consider merging pull requests if they are easy to review, but because
> I don't use this tool myself I have no way to verify complex changes.
> If you'd like to make such a change, please consider contributing some
> kind of basic automated test with a corresponding small test file.
> This repo does have GitHub Actions enabled so bootstrapping some test
> coverage is now feasible.

## Basic Instructions

If you are building from source you will need `cmake` and `make`.
On Windows systems you'll need Visual Studio or MingW.

```
cmake .
cmake --build .
```

will generate an `AtomicParsley` executable.

### Dependencies:

zlib  - used to compress ID3 frames & expand already compressed frames
        available from http://www.zlib.net

