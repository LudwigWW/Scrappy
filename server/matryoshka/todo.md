- figure showing "energy" landscape (for 2D translation)
  - could also include rotation (best over all rotations for that centroid)
- figure showing that star-shaped is **not enough**
- figure showing limitation: very thin non-convex shape
- conservative inner shell for A: thickness = ½ spacing
- conservative outer cage for B: thickness = ½ spacing
- conduct cut on A and scoop out outer cage of B
  - kindly rotate output so that cut pieces are flat side down
- theory on traditional matryoshka shape:
  - Q: what are the criteria for "trivial" nesting?
  - H: Star-shaped + height-field over cut plane. (A strict subset of this
      class are convex shapes)
  - Q: If arbitrary cut plane is allowed then does shape have to star-shaped?
  - H: Already the traditional matryoshka fails if you cut diagaonally across
    top.
- aesthetic measure of cut
  - trivial to avoid certain cuts
- interactive app mousing over location
- slow cgal version
- decorate binary search counterexample as policeman 
- try treating scale as regular dof
- justify using trivial angle of attack
- justify using trivial point on plane (centroid of B)
- use A-(σ+ε)  and B during optimization, use A \ (B+σ) during carving
  - σ: print tolerance (how much extra space needed in a cavity so B will fit)
  - ε: minimum wall thickness (minimal depth inside A of cavity)
- maximize B ↔ minimize A: optimize for "packaging" 
  - engagement ring in bunny
- Limitations:
  - features less than one pixel thick
  - binary search: scale is not always "monotonic"
  - cut plane doesn't have to be planar
    - rather straight forward to address if cut surface is _given_ (clip in
      frag shader), more interesting if optimizing over curved surface
- figure showing 2D ping-pong render + "bad codes"

+ related work
  + stackabilization
  + boxelization
  + depth peeling
  + layered depth images Francois Faure for collisions
  + depth peeling for CSG
  + nested cages
