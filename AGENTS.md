# RoboCrap — agent instructions

## Mandatory change procedure

- Mandatory C++ style defines rejection conditions; optimization, brevity and architecture never
  waive them. Preserve required behavior/compatibility and explicitly resolve any conflict.
- Before each logical code change, and after context loss, re-read Mandatory C++ style and
  Mandatory code reduction. Inspect the operation/callers. Choose in order: reuse, simplify,
  extract repetition, add necessary state ownership. Before a new helper/class/field, record its
  requirement and why reuse fails in at most three sentences in the ExecPlan or simple-task commentary.
- After implementation, separately review changed declarations, bodies and callers: signatures,
  result channels, ownership and added abstractions. Record file/line violations and fix them
  within scope before completion. Functional success or "rules checked" is insufficient evidence.
- Record findings and before/after counts in the ExecPlan or simple-task handoff. Explain required
  growth against the smaller alternative. Add no approval gate or automatic build requirement.
- The .codex hook requests this pass after C++ changes during a turn. It is a reminder, not a C++
  parser or proof of compliance. Perform the pass manually if hooks are untrusted/unavailable/fail;
  never bypass a reported violation to finish a task.

## Mandatory explanation style

- Answer the exact question first. When asked for the next step, explain only that step.
  Be concise by removing repetition, never by omitting the mechanics needed to understand it.
- Be concrete: identify the inputs and where they come from, the actual operations in order,
  and the result and where it is used. Do not substitute abstract labels such as "adaptive",
  "validation", or "decoupling" for an explanation of what the code actually does.
- Use established mathematical and project terminology; do not invent terminology. Define
  every new symbol, index, technical term and quantity immediately, including units where relevant.
  Do not use arrows as a substitute for explaining a calculation or relationship.
- Always denote vectors in explanations with lowercase, regular (non-bold) letters and an overbar,
  for example `\bar{p}`, `\bar{k}`, `\bar{r}`. Do not use uppercase or bold letters or arrow accents
  for vectors.
- Always denote axis unit vectors with lowercase e, an overbar, and the axis subscript x, y,
  or z: `\bar{e}_x`, `\bar{e}_y`, `\bar{e}_z`.
- Denote transform matrices in LaTeX as `{}^{j}A_i`, from coordinate system i to
  coordinate system j. Call the matrix a transform matrix, never calibration.
- LaTeX in chat renders only as follows; otherwise it appears as plain text. Inline math uses
  `$...$`, never `\(...\)`. Any symbol with a pre-superscript (`{}^{j}A_i`, `{}^{B}R_F`) goes only
  in a `$$...$$` display block on its own lines. Markdown table cells contain no LaTeX: use plain
  text with Unicode subscripts (X₀, Z₁) and define those symbols before the table.
- In formulas, always show all arguments on which each function depends; never omit them
  for brevity (for example, write `\bar{p}_M(\theta)`, not `\bar{p}_M`). Show dependent
  arguments explicitly: if `\theta=\theta(u)`, write `\bar{p}_L(u,\theta(u))` when using
  the two-argument definition, and `\bar{p}_M(\theta(u))` inside it. Distinguish independent
  arguments from dependent quantities and explicitly identify any fixed parameters.
- In Russian answers, use accepted Russian engineering terminology, not literal translations.
  A robot joint is "сочленение", never "сустав". Check unfamiliar terms before using them;
  if uncertain, retain and define the original technical term rather than inventing a translation.
- For proposed changes, state exactly what happens now and what calculation will replace it.
  For failures, identify the failing operation, its inputs, the rejected condition and the reason.
  Ground implementation claims in inspected code; explicitly distinguish uncertainty from fact.
- Use a concrete calculation or example when explaining an algorithm. Prefer actual project
  values; label hypothetical values. Distinguish computation time from movement duration,
  stored animation points from temporary checks and RSI samples, and measurements from estimates.
- Check each sentence before answering: nouns are code identifiers or physical things, nouns replace
  pronouns, verbs have named subjects and literal meanings, terms are defined, no calculation step
  is missing. This overrides brevity; never make the user ask twice or repeat an accepted
  explanation. Re-read this section after it changes or context is lost. Distinguish existing code
  from proposals; state how example intervals are chosen and how many values each operation returns.

## Mandatory C++ style

- Do not use C++ lambdas; use descriptively named helper functions or member methods.
- Do not write project templates (functions, classes, aliases, variables or specializations).
  Existing std, Qt and Eigen template APIs remain usable.
- Do not add project tests, test harnesses or test build targets.
- Use try/catch only for exceptions originating in external-library calls (including their std
  exceptions, such as OCCT failures). Preserve recovery/cleanup; bare throw may rethrow the same
  exception. Never define project exception types or throw new exceptions of any type. Return
  expected failures through optional/results; keep compiler exception support and third-party code.
- Return results through the return value; combine result/error when both are needed. Never use
  references, raw/smart pointers or worker-populated shared objects as output/in-out channels.
  Input references/pointers and explicit operations on existing objects (setters, scene insertion)
  are allowed; do not disguise output as mutation.
- Functions/constructors have at most four parameters; use zero to two where possible. Choose
  four only after trying to reduce the count to three or fewer: make the operation a member of
  the object that owns an input, split it into separate operations, or group values forming one
  concept, never a parameter bag to evade the limit. Caller convenience is insufficient. Record
  the attempts and why they fail briefly in the ExecPlan or simple-task handoff, not as code
  comments. Reject four-parameter signatures without that justification during the compliance pass.
- Read or change another object's private state only through its methods, even when friendship
  permits direct access. A class initializes/updates its own private members in its implementation.

## Mandatory code reduction

- If an existing operation or a smaller implementation meets the same requirements, use it.
  Remove obsolete paths, repeated calculations, redundant state and forwarding layers within scope.
  Count helpers, declarations, callers and configuration together. Moving files, shrinking only
  a caller, compressed formatting and macros hiding logic are not reductions.
- Extract repetition only when responsibility and failure semantics match. Parameterize the same
  operation with meaningful inputs; add a class only for required state/ownership/invariants.
  Reject speculative frameworks, parameter bags and abstractions without a current requirement.
- Use C++17 algorithms/container operations, structured bindings, auto, range-based for, if
  initializers and aggregate initialization when they remove boilerplate. Keep a simpler loop when
  adapters add code. Preserve value/reference semantics, Eigen evaluation and lifetimes;
  std::ranges and designated initializers require C++20 and are prohibited.
- Reduce nesting with guards; use tables for actual data mappings. Preserve evaluation order,
  side effects, first failure, defaults and invalid-input handling. Do not replace calculations
  or state transitions with a generic dispatcher merely to remove branches.
- Use compiler-generated lifetime operations for ordinary values (Rule of Zero) and existing
  resource owners for automatic cleanup (RAII). Retain required identity/zero initialization,
  borrowed-model copy/move restrictions, thread-affine destruction, shutdown/join order, rollback
  and external-library error handling.
- Validate unchanged inputs once at a proven entry point. Remove a check only after proving every
  path already checked the same value, condition, tolerance and lifetime. Check new results and
  independent boundaries as required; flange/TCP residuals, revisions and thread ownership differ.
- Capture file and physical/nonblank line counts at step start/finish, including added/deleted
  helpers, headers and callers. Separate C++, QML, build inputs and tooling from documentation/assets.
  Achieve reduction/no growth wherever requirements permit; explain necessary growth through the
  required behavior and smaller alternative considered. Keep checks, recovery, diagnostics and
  readable formatting; report source size separately from computation.

## Mandatory interface stability

- Introducing UI elements or state-dependent sizing that makes the interface jump is unacceptable.
  Showing, hiding or updating messages, progress indicators or controls must not resize the scene
  viewport, panels or windows, or displace existing controls during ordinary state changes.
  For example, showing "Select a generated machining path" must not shrink the scene viewport.
- Reserve stable layout space for transient content, replace content within a stable region, or
  use an overlay outside the parent layout. Keep control sizes stable when their text changes;
  hiding content with opacity is insufficient if text wrapping still changes its layout size.
- Inspect hidden/visible, loading/error, selection and playback states for layout shifts.
  Deliberate user resizing, navigation and requested expand/collapse actions may change layout;
  incidental status or content updates must not cause those changes.

## Scope and workflow

- Machining scope is animation preparation/playback plus nominal RSI Sampling/streaming for the
  RSI_Chamfer_Hole T1 run. Force correction stays out of scope unless the user explicitly reopens it.
- Before behavior/API changes, read affected files/callers. For changed QML components/uses,
  inspect definitions and custom base types: properties, signals, default content and sizing.
- Stay within the task; preserve unrelated edits/formatting. Reuse suitable project/Qt helpers;
  add abstractions only for a current requirement.
- Exclude directories named build, libs or python, and names beginning with old, from routine
  searches/inspections unless explicitly targeted.
- Preserve public/protected C++ APIs, QML APIs/module URIs, dependencies and minimum versions
  unless the task requires a change. Explain it and update callers. For an unworkable design,
  explain the verified conflict and a concrete alternative.
- Git staging, commits, branch switches, stashing, history rewrites or discarding changes require
  an explicit user request, including when following PLANS.md.
- Do not propose, offer or ask about commits after completing a request; the user commits when
  they decide to.
- Complex features/significant refactors require reading PLANS.md and maintaining an ExecPlan;
  simple edits, documentation-only work and read-only analysis do not. Store every task plan/
  strategy in repository-root plans/, never elsewhere.
- Edit source inputs, not generated outputs. Change vendored libs/eigen-5.0.0 or local IDE settings
  CMakeLists.txt.user* only when specifically required.
- Live PLC/FTS/RSI connections, outputs, sensor bias or motion require explicit authorization
  covering that hardware action. GUI/build checks do not grant it.

## Project basics

- Windows Qt Quick application: C++17, Qt >= 6.8, CMake >= 3.16 and bundled Eigen.
  Root/module CMakeLists.txt files are authoritative; local SDK versions do not raise these minimums.
- The repository root is `E:\Qt\QtProjects\RoboCrap` or `D:\Qt\QtProjects\RoboCrap`, depending on
  the computer the user works at; the manuals folder follows the same drive. Use
  repository-relative paths in documents and plans; check the session's working directory for
  absolute paths.
- Preserve USE_HOT_RELOAD and FAST_QML_BUILD behavior. old_imp/ is outside the active build;
  python/ contains analysis scripts/data, not the current application or an automated test suite.

## Read contracts for the affected area

Before changing or reviewing the areas below, read the matching sections of
[docs/AGENT_CONTRACTS.md](docs/AGENT_CONTRACTS.md), then the indicated producers/consumers.
They are mandatory project constraints unless the user requests a redesign. Read all matching
sections for work crossing boundaries; do not read unrelated sections for every request.
Use [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) when a broader implementation map is needed.
Update only affected constraints in their owning document. Do not add completion reports, task
history, tutorials or repeated implementation descriptions.

| Work area | Required contract sections |
| --- | --- |
| CMake, target ownership, dependencies, QML registration, modules or resources | [Build and modules](docs/AGENT_CONTRACTS.md#build-and-modules) |
| OCCT viewport, CAD loading, robot assets or presentation | [Viewport and CAD](docs/AGENT_CONTRACTS.md#viewport-and-cad) |
| Devices, protocols, device UI or thread/shutdown changes | [Devices and protocols](docs/AGENT_CONTRACTS.md#devices-and-protocols) |
| Numerical geometry, kinematics, scene objects/imports/intersections or tool transforms | [Geometry and scene](docs/AGENT_CONTRACTS.md#geometry-and-scene) |
| Machining generation, trajectory preparation, playback, diagnostics or authorized RSI work | [Machining and playback](docs/AGENT_CONTRACTS.md#machining-and-playback) and [Geometry and scene](docs/AGENT_CONTRACTS.md#geometry-and-scene) |
| QML controls/layouts, delegates, settings UI or component exports | [QML](docs/AGENT_CONTRACTS.md#qml) |

## Validation and handoff

- Inspect git status and the final diff, including new/untracked files. Run git diff --check;
  inspect untracked contents separately because ordinary diffs omit them.
- Run CMake configure/build or build-backed QML lint only when explicitly requested.
  Do not run by default or ask permission to build.
- When requested, discover the installed matching Qt/compiler kit; do not invent SDK paths.
  Before reusing a build, check CMAKE_HOME_DIRECTORY and compiler/Qt paths in CMakeCache.txt.
  Preserve its mode; use a separate directory when needed.
- USE_HOT_RELOAD=ON requires Felgo/FelgoHotReload; OFF embeds application QML and is required
  for packaging verification. FAST_QML_BUILD=ON skips cache generation; verify OFF when
  cache-generation verification is requested.
- Build the owning target for C++/CMake/resources, plus robocrap for application integration or
  concrete-device/geometry/path changes. Build for QML registration/resource changes too.
  For QML, verify available targets and run Components_qmllint, Styles_qmllint or robocrap_qmllint
  as appropriate (hot reload OFF for application QML); use all_qmllint for multiple modules.
- Check changed UI states, resizing and binding/import warnings in a disconnected runtime when
  available. Build/lint success proves neither visual nor hardware correctness.
  Documentation-only work needs content/diff checks.
- Fix introduced failures. Report pre-existing failures separately; change them only to unblock
  relevant validation. Report checks run/results and skipped checks/reasons. Reviews identify
  verified findings by file/line, impact and correction.
