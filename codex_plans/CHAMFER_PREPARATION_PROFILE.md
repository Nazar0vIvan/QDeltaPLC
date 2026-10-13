# Chamfer preparation baseline

Step 1 instruments the existing preparation without changing geometry, timing, sampling density, IK seed order or validation limits. The numerical result structures carry owned diagnostic JSON objects. The GUI coordinators print one compact Qt info message per completed worker result. There is no per-point logging or shared mutable worker result.

## Reproduction

Use the existing Felgo Qt 6.8.3 / MinGW GCC 13.1 Debug build. From the repository root, with that kit's bin directories on PATH and QT_PLUGIN_PATH set to its plugins directory:

    build/Felgo_SDK_Desktop_Qt_6_8_3_MinGW_64_bit-Debug/robocrap.exe -platform offscreen --profile-chamfer codex_plans/chamfer_profile_input.json codex_plans/chamfer_profile_debug.json

The input JSON records the latest screenshot settings: 1 mm/s machining and transfer feed, 1 mm/s² transfer acceleration, 100% lead acceleration/deceleration distance, 30 mm staging distance, 5 mm clearances, 30° spans, 0.5 mm chamfer and 45° chamfer angle. The default committed ER calibration is combined with ER-relative TCP [0,15,1.5,0,0,0]. The report records the resulting flange-relative XYZABC. The plane and cylinder are fitted from XP.json/XC.json through the application factories and intersected through intersectSurfaces. They are loaded once, before the preparation timer starts.

The diagnostic exits before Felgo initialization and device construction. It invokes the real ChamferMotion::create and PreparedChamferJob::create functions. It creates no sockets, scene objects or controller files. No correction-limit profile is supplied, so successful nominal encoding still cannot establish encoding readiness. Its JSON report is written after each completed repetition. Preview failure returns exit status 1; input/report I/O failure returns 2. RSI failure is recorded separately and does not discard the preview measurement.

## Interpretation

All durations are wall-clock milliseconds measured with QElapsedTimer. They include instrumentation overhead and operating-system scheduling. Build time, file import, application startup, rendering and GUI dispatch are excluded from numerical preparation totals. No Release performance inference is justified by Debug measurements.

The preview's top-level IK/FK counters cover configuration search, staging transfers and assembly of the final points. Central-validation counters are nested separately in each candidate attempt; add them to the top-level counters for whole-preview operation totals. IK time includes FK executed internally by Kr10Kinematics. Explicit FK time/count excludes those internal calls. Therefore IK and explicit FK times are disjoint, but both are subsets of their enclosing stage times; do not add them again to those stage totals.

Each attempt records elapsed transfer-in/out time and added points, central analytical construction (three inverse arc-length maps plus timing initialization), central sampling/validation, central assembly and its error. Failed attempts retain diagnostics. Candidate count is the number of unique starting configurations found; attempt count is the number actually compiled before success/failure. Phase interval counts omit the shared initial point; their sum plus one is the stored preview point count.

Central interpolationChecks counts temporary analytical-target/FK comparisons between retained joint samples. RSI intervalValidationSamples counts additional temporary IK/FK evaluations, including repeated controller endpoints. retainedSamples is the successful output count or the partial work reached before a rejection; a partial count is not usable motion output.

## Results

The application build and all three offline runs completed successfully. Run 1 was briefly paused by a debugger to verify progress and is excluded from the clean comparison. Preview totals for runs 1/2/3 were 66.063, 62.577, 62.451 seconds; RSI totals were 19.411, 19.422, 19.484 seconds.

For clean runs 2 and 3, mean preview preparation was 62.514 seconds. Its disjoint measured stages were analytical construction 0.471 s, central sampling/validation 33.916 s, staging transfers 26.233 s, central point assembly 1.645 s, and configuration search 0.0022 s. The small remainder covers HOME, assembly checks and bookkeeping. Failed attempts consumed 34.096 s of the total; this overlaps the stage breakdown and must not be added to it.

Mean RSI numerical preparation was 19.453 seconds: sampling/validation 19.091 s and nominal encoding 0.361 s. It produced 19,195 stored poses (initial pose plus 19,194 corrections), making 172,750 IK and 172,750 explicit FK calls. There were 153,551 temporary interval-validation evaluations. Numerical sampling and encoding succeeded; encodingReady remained false because this offline run intentionally supplied no controller correction-limit profile. No shared-data export was attempted.

Each preview run made 301,900 IK and 1,422,375 explicit FK calls across all candidates, with 905,577 temporary interpolation checks. All runs retained the same 132,067 points and the same phase counts: HOME out 149 intervals, transfer in 27,535, lead-in 14,248, machining 48,280, lead-out 14,248, transfer out 27,535, HOME return 71. Include one shared initial point for the total. Playback duration is 140.8837897766 s; central duration is 76.7721997553 s. These are motion durations, distinct from computation times.

Validation: full robocrap Debug build passed with existing Felgo configuration and CMake deployment policy warnings; git diff --check passed. New source files were reviewed for style and whitespace. No project tests were added, no equipment was connected, and no UI playback validation was claimed.

## Verified source of repeated work

For the supplied case the search finds four unique staging configurations and tries all four. The first three are rejected during central validation at approximately 21.550904 s (A4), 44.609803 s (A6), and 21.252918 s (A6). Reported interval speeds are approximately 360,000 deg/s, consistent with a 360° winding change across a 1 ms interval. The solver's resolveJointAngle clamps the nearest equivalent turn to the joint's allowed range; the validator rejects the discontinuity. These are rejected candidate paths, not a speed spike in the accepted animation. This step does not change that behavior.

Every attempt first builds the same-duration incoming staging transfer with a different joint configuration. Each adds 27,535 points. The first three attempts then calculate 21,549, 44,609 and 21,253 retained central samples before rejection. The fourth accepts all 76,777 central samples, recomputes FK when assembling them into ChamferMotion, builds another 27,535-point outgoing transfer, and completes HOME timing. The final stored motion has 132,067 points.

The result supports both adaptive preview sampling and avoiding expensive full-path attempts before cheap configuration screening. Moving map construction outside the candidate loop alone saves only about 0.35 seconds in this example; it cannot fix a minute-long Generate. Steps 3 and 4 address the dominant repeated dense IK/FK work. No new winding-selection algorithm was implemented as part of profiling.
