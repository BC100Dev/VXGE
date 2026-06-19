# VXGE: Vulkan-powered Game Engine
VXGE (Vulkan-powered Game Engine) is a C++ game engine library built on top of SDL3 and
Vulkan, designed to abstract the Vulkan rendering pipeline away from game code so
developers can focus on building games rather than managing GPU infrastructure.

Yes, this is LLM-assisted product with active testing, making sure that everything WILL
work, no matter how much those clankers will try to sabotage it. The reason for me
vibe-coding is that I barely have any experience with Vulkan and I genuinely need a game
engine for my upcoming games, including the one that I'm still planning. That big game is
shown [in here](https://github.com/BC100Dev/BC100Dev/blob/main/Big_Project_Spoilers.md).

## Usage
If you want to see the active tests and how to write a custom implementation of yourself,
[the base test game](Sources/VXGE_TestExec) is your best bet in learning, how this works
and how you can replicate things on your own. Alternatively, you can pass in a `SKILL.md`
to your clanker, and it'll produce working code... well, hopefully, I let Claude connect
to my system using the `filesystem` and `bash-shell` connectors because I'm a vibe-coder,
like everyone else because it just makes things faster, no?

And yes, I let Claude Sonnet 4.6 write the [SKILL](SKILL.md) file because I've been using
it the entire time for the coding sessions.

## Why should I use VXGE?
VXGE gives you complete ownership of your rendering pipeline with zero abstraction tax
of some sort. When you call onto a renderer submission, you know EXACTLY what Vulkan
calls are being fired underneath it. You genuinely own the engine because I, as BC100Dev,
do not charge any fees, like Unity does, that was struck in 2023. UE takes 5% of gross
revenue past $1 million. VXGE is LGPL licensed, meaning that your game code stays yours,
along with no licensing fees and no specific terms that can be changed under your feet
by a corpo. Along with that, VXGE is community-maintained, meaning that anyone can expand
VXGE, whether through APIs or VXGE's internal codebase itself. The engine gets better
with contributors that actively test code out.

Moreover, it is straight C++ instead of Mono/IL2CPP or a Blueprint system. VXGE is
written in C++ with a Vulkan abstraction and nothing else, meaning that the runtime is
not managed. VXGE is designed to be a native library, along with being a simple, yet
effective library that can be used by any C++ developer.

Hate bloat? VXGE is designed to ship as small as possible. Unreal Engine? It cranks up
to gigabytes really fast. And if you think that coding coordinates is difficult, VXGE
will also have an optional level editor, meaning you can edit your maps and other
level-based files directly with a standalone executable.

Want to be a better developer, even if you vibe-code? VXGE will make sure that you know
what swapchains are, what a descriptor set does, or why frame pacing matters. Unity and
UE abstracted it for simplicity's sake.

## Why should I NOT use VXGE?
Of course, VXGE is not meant for everyone, and it is primarily meant to be written with
C++. Knowing people, you all want simpler languages.

One is the obvious one, development speeds. If you want to ship fast, Unity and Godot are
meant to be built quickly. VXGE also does not have an asset store or things like these,
except for a simple level editor that is yet planned but not in active development just
yet.

If you or your team is not C++ native, VXGE will be fighting you guys because there is
no scripting layer or a managed runtime that Unity / UE / Godot would give you.
Everything is C++, meaning if your team uses other languages, VXGE is not the right tool
for you :sob:

In case you need a mature ecosystem, like having a plugin integration or other things,
VXGE is not the thing for you just yet. Physics, networking, audio middleware, platform
certification tooling, and all that exist as mature solutions for UE and Unity, and as
of now, with VXGE, it needs more work... for now because later on in development, VXGE
will have the capability to do Steam Networking communications, simple physics, and the
usual stuff Unity provides.

If you are a complete beginner in C++, please don't start with VXGE because it assumes
you already understand C++. Moreover, VXGE assumes you already know, how GPU pipelines
work, being comfortable debugging Vulkan errors and having knowledge of writing shaders.
Sorry, it's not a learning engine, but it is an engine that you can study because it is
meant to be open source.