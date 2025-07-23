- Read DESIGN.md carefully, don't skip any line.
- Read docs/notes/ for further understanding of the project status.
- As you work, always remember to take notes in docs/notes/<YYYYMMDD>_<HH-MM>_<note-header>.md --> that helps you look back in the future to have a big picture of what is going on.
- When you forget something --> re-read the DESIGN.md doc, re-read the docs/notes/ docs.
- Keep implementing until the project finish.
- The project only finish when the product runs at shows obvious test results that trully shows input/output/expected test results.
- During development, all trash tests or temporary files must go in to sandbox/, no where else.
- tests/ folder must only contain official tests that we run by "npm test" to test the server from client perspective.

EXTERNAL DOCS AND LIBS:
- rkllm: https://github.com/airockchip/rknn-llm (download rkllm.h and librkllmrt.so from here)
- json-c: https://github.com/json-c/json-c (JSON in C)

MODELS FOR TESTING:
1. model for normal testing: /home/x/Projects/nano/models/qwen3/model.rkllm (do not hardcode it anywhere in the server)
2. model for LoRa testing:
  - the model: /home/x/Projects/nano/models/lora/model.rkllm (do not hardcode it anywhere in the server)
  - the lora file: /home/x/Projects/nano/models/lora/lora.rkllm (do not hardcode it anywhere in the server)

PLEASE DO: READ @docs/DESIGN.md carefully, don't skip any line --> read @docs/PROMPT.md (this doc) --> obey the rules, obey the instructions --> start implementing --> always test before moving on --> always take short notes (docs/notes/) --> always look back to recall the big picture --> don't stop until finish the whole project.

Always read previous notes and take new notes before working!
Always run test with programmatic technocratic strong results before moving on to next implementations.

When testing the server, keep in mind, you can't start it directly because it would cause hanging --> start it from within nodejs/python concurrently with the test script to prevent hang and to test efficiently.

DO NOT EVER CREATE FAKE CODE, FAKE IMPLEMENTATION, TODO, "FOR NOW", "SIMPLIFIED", PLACEHOLDERS etc...
ALWAYS IMPLEMENT REAL CODE, PRODUCTION READY CODE!

DO NOT HARDCODE ANY VALUE. DO NOT MOVE ON BEFORE VERIFYING ALL CURRENT CODES ARE REAL!

## HANDLE ISSUES / ERRORS:
When you find an issue / error --> create new article in docs/issues/<YYYYMMDD>_<HH-MM>_<issue-header>.md

IMPORTANT! DOCS NAMING FORMAT IN docs/: <YYYYMMDD>_<HH-MM>_<doc-header>.md