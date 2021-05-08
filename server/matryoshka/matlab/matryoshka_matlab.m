function [VL,FL,JL,VT,FR,JR] = matryoshka_matlab(VA,FA,VB,FB,p,np)
  [VL,FL,JL1] = half_space_intersect(VA,FA,p,np);
  [VR,FR,JR1] = half_space_intersect(VA,FA,p,-np);
  bbd = normrow(max(VB)-min(VB));
  [SVL,SFL,SJL] = linear_sweep(VB,FB, bbd*10*np);
  [SVR,SFR,SJR] = linear_sweep(VB,FB,-bbd*10*np);
  [VL,FL,JL] = mesh_boolean(VL,FL,SVL,SFL,'minus');
  [VR,FR,JR] = mesh_boolean(VR,FR,SVR,SFR,'minus');
  JL(JL<size(FAL,1)) = JL1(JL(JL<size(FAL,1)));
  JR(JR<size(FAR,1)) = JR1(JR(JR<size(FAR,1)));
end
