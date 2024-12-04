# CloudVocal - Professional Cloud AI Transcription & Translation Plugin

<div align="center">

[![GitHub](https://img.shields.io/github/license/locaal-ai/obs-cloudvocal)](https://github.com/locaal-ai/obs-cloudvocal/blob/main/LICENSE)
[![GitHub Workflow Status](https://img.shields.io/github/actions/workflow/status/locaal-ai/obs-cloudvocal/push.yaml)](https://github.com/locaal-ai/obs-cloudvocal/actions/workflows/push.yaml)
[![Total downloads](https://img.shields.io/github/downloads/locaal-ai/obs-cloudvocal/total)](https://github.com/locaal-ai/obs-cloudvocal/releases)
[![GitHub release (latest by date)](https://img.shields.io/github/v/release/locaal-ai/obs-cloudvocal)](https://github.com/locaal-ai/obs-cloudvocal/releases)
[![GitHub stars](https://badgen.net/github/stars/locaal-ai/obs-cloudvocal)](https://github.com/locaal-ai/obs-cloudvocal/stargazers/)
[![Discord](https://img.shields.io/discord/1200229425141252116)](https://discord.gg/KbjGU2vvUz)
<br/>
Download:</br>
<a href="https://github.com/locaal-ai/obs-cloudvocal/releases/latest/download/obs-cloudvocal-windows-x64-Installer.exe"><img src="https://img.shields.io/badge/Windows-0078D6?style=for-the-badge&logo=windows&logoColor=white" /></a>
<a href="https://github.com/locaal-ai/obs-cloudvocal/releases/latest/download/obs-cloudvocal-macos-x86_64.pkg"><img src="https://img.shields.io/badge/mac-000000?style=for-the-badge" /></a>
<a href="https://github.com/locaal-ai/obs-cloudvocal/releases/latest/download/obs-cloudvocal-x86_64-linux-gnu.deb"><img src="https://img.shields.io/badge/Linux-FCC624?style=for-the-badge&logo=linux&logoColor=black"/></a>
</div>

## Introduction

CloudVocal brings professional-grade cloud transcription and translation to your OBS streams and recordings. Powered by industry-leading cloud providers, it delivers exceptional accuracy and real-time performance for your live streaming needs. ✅ Professional-grade accuracy, ✅ support for 100+ languages, ✅ enterprise-level reliability, and ✅ blazing-fast performance!

If this plugin has been valuable consider adding a ⭐ to this GH repo, rating it [on OBS](https://obsproject.com/forum/resources/authors/royshilkrot.319842/), subscribing to [my YouTube channel](https://www.youtube.com/@royshilk) where I post updates, and supporting my work on [GitHub](https://github.com/sponsors/royshil), [Patreon](https://www.patreon.com/RoyShilkrot) 🙏

CloudVocal integrates seamlessly with leading cloud providers to deliver enterprise-grade speech recognition and translation services. Simply configure your API credentials and start streaming with professional-quality captions and translations.

## Features

Current Features:
- Professional-grade transcription with industry-leading accuracy
- Real-time translation using enterprise cloud translation services
- Support for 100+ languages with dialect recognition
- Streaming-optimized performance with minimal latency
- Multiple cloud provider options for transcription and translation
- Caption output in multiple formats (.txt, .srt, .vtt)
- Sync'ed captions with OBS recording timestamps
- Direct streaming to platforms (YouTube, Twitch) with embedded captions
- Advanced text filtering and customization options
- Partial transcriptions for a streaming-captions experience
- Custom vocabulary and pronunciation support
- Professional terminology handling for specific industries

Roadmap:
- Speaker diarization for multi-speaker environments
- Advanced profanity filtering options
- Custom translation glossaries
- Additional subtitle format support
- Enhanced analytics and caption quality metrics

## Usage

Tutorial videos and screenshots - coming soon!

## Download and Installation

Check out the [latest releases](https://github.com/locaal-ai/obs-cloudvocal/releases) for downloads and install instructions.

### Configuration

1. Download and install the appropriate version for your operating system
1. Add CloudVocal as a filter to your audio source
1. Configure your cloud provider credentials in the plugin settings
1. Select your desired transcription and translation options
1. Select an output text source for the captions and translations, send the captions to the stream or a file

## Building

The plugin can be built on Windows, macOS, and Linux platforms. The build process is straightforward as all processing happens in the cloud.

Both Mac OSX and Linux rely on Conan for dependencies. Install Conan, e.g. `pip install conan`, and install the dependencies:
```sh
$ conan profile detect --force
$ conan install . --output-folder=./build_conan --build=missing -g CMakeDeps
```

### Mac OSX

```sh
$ ./.github/scripts/build-macos --config Release
```

### Linux

```sh
$ ./.github/scripts/build-linux
```

### Windows

```powershell
> .github/scripts/Build-Windows.ps1 -Configuration Release
```

If you're developing the plugin, I find this command to be useful for direct deploymet into OBS after building:

```powershell
> .\.github\scripts\Build-Windows.ps1 -Configuration RelWithDebInfo -SkipDeps && Copy-Item -Force -Recurse .\release\RelWithDebInfo\* "C:\Program Files\obs-studio\"
```

### Other Plugins 

Check out our other plugins:
- [LocalVocal](https://github.com/locaal-ai/obs-localvocal) for on-device real-time transcription and translation
- [Background Removal](https://github.com/locaal-ai/obs-backgroundremoval) for removing background from live portrait video
- [Detect](https://github.com/locaal-ai/obs-detect) for real-time on-device object detection
- [CleanStream](https://github.com/locaal-ai/obs-cleanstream) for real-time profanity filter
- [URL/API Source](https://github.com/locaal-ai/obs-urlsource) for real-time data integrations
- [Squawk](https://github.com/locaal-ai/obs-squawk) for real-time on-device speech generation (text-to-speech)
