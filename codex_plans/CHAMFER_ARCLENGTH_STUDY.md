# Distance-to-parameter numerical study

Research date: 2026-10-10. Scope: numerical study and source inspection only. No application build, execution, C++ changes, project test harness, shared-job export or robot operation.

## Recommendation

For RoboCrap's repeated arbitrary-distance evaluations, the preferred engineering candidate is an adaptively refined piecewise quintic Hermite approximation of the inverse arc-length function, constructed once for each analytical phase. Use accurate derivative-based integration for the stored distances, match first and second inverse derivatives at interval endpoints, and validate distance, speed and acceleration errors before accepting an interval. Retain safeguarded local Newton/integration as a fallback or independent reference. This is a project-specific recommendation, not a proof of global optimality or a measured C++ performance claim.

For a single sequential sample stream, adaptive Runge-Kutta is a strong alternative and can cost less overall than constructing a highly accurate reusable inverse. An adaptive high-order ODE solver with dense output is another credible candidate; it was researched but not benchmarked here. The earlier suggestion to start with sequential RK4 was incomplete because this code also makes arbitrary intermediate and repeated evaluations.

## Actual project requirements

chamfercentralmotion.cpp converts prescribed time into distance and then calls progressAtDistance. chamfercentralvalidation.cpp evaluates 4 ms intervals, quarter samples and interpolation-check locations, with further adaptive refinement. chamfercartesiansampler.cpp revisits the analytical evaluator at 4 ms samples, saved knots, internal validation points and phase/ramp boundaries. These operations do not form one simple forward-only query stream. Geometry is immutable within a saved motion, so a distance conversion can be constructed once and reused. Dry Run itself currently interpolates saved joints and performs FK; it does not need a new inverse solve on every display refresh.

The existing chamferarclength.cpp sums chords and linearly interpolates parameter against distance. A positional error bound alone does not bound derivatives of the inverse. This explains why tiny XYZ errors can still trigger strict speed checks. Existing rate checks allow a relative excess of 1e-6 and use acceleration estimates over intervals down to approximately 100 microseconds. Joint/XYZ validation, Euler continuity, correction limits and exact endpoint handling remain independently necessary.

## Definitions and proposed calculation

Let p(u) be the analytical TCP-origin position in millimetres for phase parameter u in [0,1]. Let g(u)=norm(p'(u)), the distance covered per unit parameter. Define S(u)=integral from 0 to u of g(v) dv. Let s be the required travelled distance and F(s) the inverse parameter, so S(F(s))=s.

At a selected parameter u_i, compute s_i=S(u_i), F(s_i)=u_i, F'(s_i)=1/g(u_i), and F''(s_i)=-g'(u_i)/g(u_i)^3. Subscript i labels a stored interval endpoint. Integrate g directly rather than reconstructing it from full TCP poses or summing chords. The current code unnecessarily evaluates rotations while measuring XYZ arc length; a position-derivative helper can avoid that work.

Between two endpoint distances, construct a fifth-degree polynomial matching parameter, first derivative and second derivative at both ends. A fifth-degree polynomial has six coefficients, matching these six conditions. Adjacent intervals share all three endpoint values. Thus F, F' and F'' are continuous across their shared boundary. This continuity does not by itself establish accuracy or monotonicity.

For each requested distance: locate its interval, evaluate the polynomial, then call the existing analytical position/orientation evaluator at that parameter. The polynomial approximates only the distance conversion, not XYZ geometry or orientation. Normal accepted-interval evaluation needs no integration or root search. Construction starts with known u values and integrates their distances; it does not require solving an inverse problem for every knot.

For an interval starting at distance s_a and ending at s_b, use normalized local coordinate x=(s-s_a)/(s_b-s_a). Evaluating F(s) is then evaluating a stored polynomial in x. Scale the parameter difference locally too, to reduce cancellation in coefficients.

## Accuracy criteria

Let Q(s) be the approximate inverse. Three distinct checks are useful:

- Distance residual: S(Q(s))-s, measured in mm.
- Speed ratio: g(Q(s))*Q'(s), which should equal 1.
- Derivative of that speed ratio: g'(Q(s))*Q'(s)^2 + g(Q(s))*Q''(s), which should equal 0.

If requested scalar speed is v and tangential acceleration is a, actual tangential acceleration along the analytic curve is a times the speed ratio plus v squared times the derivative of that ratio. This supplies a direct check for unwanted acceleration from the approximate parameter conversion. Curvature acceleration, orientation derivatives and joint-space constraints still need separate checks.

The research prototype refined intervals using distance residual 1e-10 mm, relative speed-ratio error 1e-8 and speed-ratio derivative error 1e-7 per mm. At 5 mm/s, the latter corresponds to 2.5e-6 mm/s squared of additional tangential acceleration. These are experimental numerical tolerances, not robot positioning accuracy and not final production settings. Production tolerances should be scaled to selected feeds, acceleration settings, coordinate magnitude and the validation margin.

Check positivity of Q' over the whole interval, not just endpoints. Since Q' is a quartic polynomial, its minimum can be inspected using endpoints and the real roots of Q''. Check position/orientation and sampled rates independently, including exact phase boundaries and endpoints. Internal sampling for refinement is evidence, not an analytic guarantee on g(Q). Bound all refinement work and reject unresolved or ill-conditioned input rather than silently forcing the last point or loosening physical limits. Exact endpoint pinning must be consistent with the length used by the speed profile.

Adaptive Gauss-Kronrod is a reasonable production integration method for smooth regular phases, with an error estimate and subdivision. A safeguarded Newton solver uses that same integral and the positive derivative g to refine within a known parameter interval; use bisection when Newton leaves the interval. Do not integrate repeatedly from the beginning of the entire phase: use stored cumulative lengths and a small local interval.

## Experiments and limitations

Standalone Python research scripts were stored in the Windows temporary directory, outside the repository. NumPy supplied Gaussian quadrature nodes; SciPy was unavailable and was not installed. No project build or test target was created. The mathematical model reproduces the current ChamferPath TCP-origin formulas for circular cylinders intersected by tilted planes, including chamfer midpoint offset and the current lead clearance law. Cylinder axis is Z and tilt is in the XZ plane; rigid transformations do not change arc length. Chamfer size is 0.5 mm in all cases.

The analytic first derivative was evaluated directly. A complex-step derivative of the analytic scalar g supplied g' for research; production should derive it analytically or use an appropriately validated derivative implementation. Reference distances used 32-point Gauss-Legendre integration on 512 parameter intervals, cross-checked against 64-point integration; total-length disagreement was at most 2.9e-14 mm. This is a numerical cross-check, not exact arithmetic. Residuals below about 1e-13 mm should be treated as the numerical floor rather than meaningful rankings.

The prototypes compared current chord/linear mapping, derivative-matched cubic and quintic maps, local safeguarded Newton with integration, fixed RK4 and adaptive RK4 with step doubling. The Newton prototype used fixed 16-point local quadrature; its integration error was not independently controlled, so it is not the complete recommended robust implementation. The quintic prototype used 16-point quadrature, recursively refined and independently checked against the higher-order reference. Initial cubic trials required many more intervals for the derivative criteria; no polished cubic/PCHIP or Chebyshev implementation was benchmarked.

The five cases were: radius 10 mm with a flat opening and default lead; radius 10 mm with a 30-degree tilted opening for machining; the same tilt for lead-out; a 60-degree tilted opening with a 20-degree chamfer angle, 2 mm clearance and 90-degree lead span; and radius 2 mm with a flat opening, 0.05 mm clearance and 120-degree lead span. Default chamfer angle is 45 degrees, clearance 5 mm, span 30 degrees. These are geometry cases, not verified reachable KR10 jobs. Some demanding cases may fail physical acceleration or IK checks regardless of the distance-conversion algorithm.

Constant-speed experiments isolate conversion accuracy. A second experiment used the project's 30-percent lead ramp law and 5 mm/s machining speed. Error is calculated as independent reference arc length at the returned parameter minus requested distance. Speed error is the change in that residual divided by 4 ms; tangential acceleration error is its second difference divided by (4 ms)^2. These are arc-distance errors, not chord-speed shortfall, and not complete XYZ or joint acceleration errors. Regular 4 ms intervals were measured separately from exact endpoint checks.

## Results with the acceleration/deceleration profile

All figures below are maximum absolute errors on the sampled calculation. No timing result includes IK, FK, quaternion evaluation, encoding or GUI work.

| Case | Method | Distance error mm | Speed error mm/s | Tangential acceleration error mm/s^2 |
|---|---|---:|---:|---:|
| flat_lead | quintic | 9.03e-11 | 3.07e-09 | 2.09e-07 |
| flat_lead | Newton | 9.65e-13 | 2.4e-10 | 8.89e-08 |
| flat_lead | RK4 | 1.14e-13 | 4.44e-13 | 1.67e-10 |
| flat_lead | RK4_adaptive | 8.88e-15 | 4.44e-13 | 2.22e-10 |
| tilt30_machining | quintic | 9.36e-11 | 1.39e-09 | 7.51e-08 |
| tilt30_machining | Newton | 8.14e-13 | 2.03e-10 | 1.01e-07 |
| tilt30_machining | RK4 | 1.71e-13 | 3.55e-12 | 1.78e-09 |
| tilt30_machining | RK4_adaptive | 1.63e-13 | 3.55e-12 | 1.78e-09 |
| tilt30_leadout | quintic | 8.51e-11 | 5.46e-09 | 7.05e-07 |
| tilt30_leadout | Newton | 8.31e-13 | 2.08e-10 | 1.02e-07 |
| tilt30_leadout | RK4 | 6.32e-13 | 2.67e-12 | 2.22e-10 |
| tilt30_leadout | RK4_adaptive | 3.29e-14 | 4.44e-13 | 2.22e-10 |
| tilt60_lead | quintic | 8.68e-11 | 5e-09 | 6.33e-07 |
| tilt60_lead | Newton | 9.92e-13 | 2.48e-10 | 9.96e-08 |
| tilt60_lead | RK4 | 1.62e-11 | 2.44e-11 | 4.44e-10 |
| tilt60_lead | RK4_adaptive | 6.22e-14 | 8.88e-13 | 4.44e-10 |
| short_clearance | quintic | 8.75e-11 | 6.52e-09 | 1.85e-06 |
| short_clearance | Newton | 7.43e-12 | 5.36e-10 | 1.04e-07 |
| short_clearance | RK4 | 7.57e-08 | 1.13e-05 | 0.00385 |
| short_clearance | RK4_adaptive | 5.77e-15 | 2.22e-13 | 1.11e-10 |

Fixed RK4 is exceptionally accurate on several cases but, with the actual ramp law on the short-clearance case, its speed error reaches 1.13e-5 mm/s, above the existing 5e-6 mm/s allowance at 5 mm/s. Adaptive RK4 resolves this. This does not imply every ODE method fails: the problem is treating a fixed step with no error estimate as sufficient for all permitted geometry.

Across the five quintic cases, sampled distance error stayed below 9.4e-11 mm and sampled speed error below 6.6e-9 mm/s under the actual profile. The largest sampled tangential acceleration error was about 1.85e-6 mm/s squared. Dense derivative evaluation at 41 points per polynomial interval found relative speed error below 1.83e-9 and the reported tangential-acceleration bound below 2.46e-6 mm/s squared. End-distance residuals were below 4e-14 mm. Separate 100-microsecond samples centered on map boundaries gave maximum spurious tangential accelerations of 8.9e-8, 1.43e-6 and 1.16e-6 mm/s squared for flat lead, tilted machining and short-clearance cases respectively.

These observations establish promise on the sampled cases, not guaranteed bounds over every supported geometry or correctness of a future C++ implementation. The prototype did not certify positivity by polynomial root isolation; dense checks found positive derivatives. That certification belongs in production validation.

## Cost and reuse

Measured timings are Python prototype wall times on this host; they are not predictions for optimized C++ or real-time scheduling. Query timing excludes error measurement. Different stopping criteria give different accuracy, so the table is a tradeoff comparison, not an equal-error contest.

For the 30-degree tilted machining curve, a median of three runs gave:

| Method | One-time construction | 50,000 arbitrary-distance queries | Total |
|---|---:|---:|---:|
| Adaptive quintic inverse | 57.4 ms | 42.9 ms | 100.3 ms |
| Local quadrature/Newton | 0.68 ms | 3296.5 ms | 3297.2 ms |

The quintic inverse used 124 intervals. A single forward pass of 3478 queries cost approximately 2.9 ms after construction. Fixed RK4 cost approximately 27 ms and adaptive RK4 approximately 91 ms for that sequential pass. Therefore fixed RK4 was faster overall for this single easy pass, while the inverse function benefited strongly from reuse and arbitrary access. Adaptive high-order ODE dense output could improve this comparison; it remains unmeasured.

Quintic construction cost ranged from about 11 ms for the default flat lead (22 intervals) to about 467 ms for the short-clearance case (819 intervals). The harder case needs enough reuse to repay construction; no universal performance winner exists. Real C++ profiling must separate map construction, map queries, IK/FK and orientation work before attributing full preparation time to arc-length inversion.

## Implementation direction if approved

Keep ChamferCentralMotion's existing prescribed distance law. Replace chord accumulation with derivative-based arc-length integration, add a validated smooth inverse per phase, and reuse it for generation and RSI preparation. Keep a bracketed local solve for difficult intervals or verification. Cache construction within each immutable geometry snapshot instead of reconstructing it for every candidate IK branch. MotionCompiler currently calls validateChamferCentral for each candidate and each call constructs the central map again; moving geometry-only preparation outside that loop can avoid repeated work independently of the chosen numerical method.

Do not interpolate XYZABC with these polynomials. Evaluate the original analytic pose at the returned u, retain existing IK/FK checks and Euler encoding, and preserve independent phase endpoints and the final RSI cycle semantics. No new Boost or SciPy application dependency is required merely to implement these algorithms.

## Primary references

David Eberly, Moving Along a Curve with Specified Speed (2023 revision): https://www.geometrictools.com/Documentation/MovingAlongCurveSpecifiedSpeed.pdf . Covers inversion by bisection and safeguarded Newton and specified-speed traversal.

Boost.Math Gauss-Kronrod documentation: https://www.boost.org/doc/libs/latest/libs/math/doc/html/math_toolkit/gauss_kronrod.html . Documents adaptive quadrature, error estimates and limitations.

Boost.Math quintic Hermite documentation: https://www.boost.org/doc/libs/latest/libs/math/doc/html/math_toolkit/quintic_hermite.html . Documents matching values/first/second derivatives and C2 continuity.

SciPy solve_ivp documentation: https://docs.scipy.org/doc/scipy/reference/generated/scipy.integrate.solve_ivp.html . Describes adaptive RK methods and dense output; recommends DOP853 for high-precision nonstiff integration.

SciPy PchipInterpolator documentation: https://docs.scipy.org/doc/scipy/reference/generated/scipy.interpolate.PchipInterpolator.html . Monotonicity preservation does not imply second-derivative continuity.

Wang, Kearney and Atkinson, Arc-Length Parameterized Spline Curves: https://homepage.cs.uiowa.edu/~kearney/pubs/CurvesAndSurfacesArcLength.pdf . Supports the broader precomputation tradeoff, but their geometry-approximating method is not the exact proposed inverse-only construction.

Research scripts and raw results for reproducibility remain at C:/Users/vano8/AppData/Local/Temp/robocrap_arc_study.py, robocrap_arc_study.json, robocrap_arc_profile_study.py and robocrap_arc_profile_study.json. These temporary files are not project tests and may be removed by normal system cleanup.
