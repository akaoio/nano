# Development Sandbox

This area is for developers to test experimental features, prototypes, and rapid development iterations!

## Purpose

The sandbox directory serves as a safe space for:
- 🧪 **Experimental Features**: Test new functionality before integrating into main codebase
- 🚀 **Rapid Prototyping**: Quick proof-of-concepts and ideas
- 🔬 **Integration Testing**: Test RKLLM server interactions with various clients
- 🛠️ **Development Tools**: Temporary scripts and utilities for development workflow
- 💥 **Breaking Changes**: Test potentially disruptive changes without affecting production code

## Guidelines

- **All temporary tests during development MUST go here!**
- Keep experiments isolated and well-documented
- Use descriptive filenames and include brief comments
- Clean up completed experiments periodically
- Feel free to nuke and rebuild - this is your playground!

## Structure

```
sandbox/
├── README.md           # This file
├── experiments/        # Feature experiments
├── prototypes/         # Quick prototypes
├── clients/           # Test client implementations
├── scripts/           # Development utilities
└── tmp/               # Truly temporary files
```

## Usage

```bash
# Navigate to sandbox
cd sandbox

# Create your experiment
mkdir my_experiment
cd my_experiment

# Code away! 🚀
```

Remember: This is rocket science testing ground - expect things to explode! 💥