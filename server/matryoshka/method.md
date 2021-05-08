## Method

Given a shape A and another shape B, the goal is to find a similarity transform
T that _not only_ places B entirely inside of A, but admits an
intersection-free, translational path _into_ A. The shape A may be _cut_ once
by a plane and the translational paths of B into the "top half" of A and the
"bottom half" of A may differ.

All told the degrees of freedom are:

 - c: 3d position of centroid of B in A,
 - θ: 3d rotation of B,
 - p,n: 2d cut plane splitting A in two,
 - a₁: out of cut plane translation of B from top half of A,
 - a₂: out of cut plane translation of B from bottom half of A, and
 - s: scale of B.

Our ultimate goal is to maximize the scale:

\\[
max_{c,θ,p,n,a₁,a₂,s} s \\
subject to ( T(c,θ,s; B) sweep along a₁ ) ∩ ( A ∩ half-plane(p,+n) ) = ∅ 
subject to ( T(c,θ,s; B) sweep along a₂ ) ∩ ( A ∩ half-plane(p,-n) ) = ∅ 
\\]

We cast this as a _maximization_ problem of the scale with respect to all other
degrees of freedom.

\\[
max_{c,θ,p,n,a₁,a₂} f(c,θ,p,n,a₁,a₂) \\
where
f(c,θ,p,n,a₁,a₂) = max_s s
                   subject to ( T(c,θ,s; B) sweep along a₁ ) ∩ ( A ∩ half-plane(p,+n) ) = ∅ 
                   subject to ( T(c,θ,s; B) sweep along a₂ ) ∩ ( A ∩ half-plane(p,-n) ) = ∅ 
\\]

Because the maximum scale occurs precisely when B is in contact with A, the
landscape of this maximization problem is rugged with discontinuities and local
maxima. We apply a grid-search/genetic algorithm/p-swarm optimization that
relies only on a fast fitness function evaluation f.

### Fitness function

For sphere topology shapes, the feasibility of s is a monotonic function: from
0 to s* it's feasible, at s* there is some contact with A, and from then on s*
to ∞ it's infeasible.

This means we can find the maximum s (i.e, we can evaluate f) for a given set
of parameters c,θ,p,n,a₁,a₂ by binary search:

```
while u-l > tolerance
  s = ½(u+l)
  if is_feasible(A,B,c,θ,p,n,a₁,a₂,s)
    l = s
  else
    u = s
```

#### Constructive (Slow) evaluation

Exact evaluation of the feasibility of B embedded in A given parameters
A,B,c,θ,p,n,a₁,a₂,s is possible. Using the robust swept volumes of [Zhou et
al.]...

#### Fast evaluation

We can quickly determine the feasibility of a given set of parameters
c,θ,p,n,a₁,a₂,s by using a GP-GPU approximation akin to depth peeling or
screen-space CSG algorithms.

We peel off front-most fragments of rendering A and the transformed B both
clipped by the plane (p,+n) (and then wlog (p,-n)) along an orthographic
viewing direction aligned with a₁ (and then a₂). For each pixel we record
whether a fragment from A or the transformed B or the background ∅ has been
drawn. If that pixel currently contains a fragment from B but on the previous
peel contained a fragment from A, then that fragment of B is _occluded_ by A
and cannot "escape" along this trajectory: this scale is infeasible. Similarly
if the current pixel contains a null fragment (background) but just previously
contained a fragment from B, then by the assumption that A is solid (and by
extension its intersection with a half space (p,+n) is solid) this fragment of 
B lies outside of A implying that this scale is infeasible.

```
D₁,D₂ ← 0
repeat
   // Each pixel of Ii contains either A,B,ø
  [Di,Ii] = peel_next_layer(Di-1)
  if ∃ pixel p such that 
    Ii(p)==B and Ii-1(p) == A
    or
    Ii(p)==ø and Ii-1(p) == B
  then
    return false
  if all Ii(p) == ∅ ∀ pixels p then return true
```

Depth peeling is accomplished in the fragment shader by passing the previous
iterations depth buffer as a texture and testing that incoming fragment's depths
are greater than previous depths. In practice, if a fragment is from A we set
the red channel to 1, and if from B then the green channel.

An occlusion query (`GL_ANY_SAMPLES_PASSED`) is used to determine if enough
layers have been peeled: if no fragments are drawn this can be the last
iteration (we still need to check feasibility on this layer because a ne
background pixel on this layer may reveal part of B outside of A).

Per-pixel feasibility tests are conducted by rendering a full-"screen"
rectangle reading from the color and depth buffers and the previous iteration's
color and depth buffers. Depth buffers are used to reveal whether the pixel
belongs to a foreground object (A or B) or the background (∅).

Note: It is assumed that back face culling is _off_.
