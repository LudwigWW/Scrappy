%[VA,FA] = load_mesh('../examples/C.obj');
%VB = VA;
%FB = FA;
%model = [
%  0.02446397 0.09714132 -0.1345878   2.538623
%  -0.1092515  0.1118409 0.06086474  0.2744559
%  0.1249577 0.07876532 0.07956387   3.202272
%  0          0          0          1
%];
%p = [
%  2.333266 0.4165727  3.391142
%];
%np = [
%  -4.675489e-17  1.131882e-16            -1
%];
%%s = 0.167776;
%s = 0.16;
%
%VBT = VB*model(1:3,1:3)' + model(1:3,4)';
%[VH,FH,JH] = half_space_intersect(VA,FA,p,np);
%[VT,FT,JT] = half_space_intersect(VA,FA,p,-np);
%FT = FT(JT~=0,:);

msoft = {'DiffuseStrength',0.2,'SpecularStrength',0.1,'AmbientStrength',0.6,'SpecularExponent',100};
clf;
hold on;
ts = {};
tsurf(      FB,VBT,'CData',2*ones(size(VBT,1),1),'EdgeColor','none',msoft{:});
ts{end+1} = tsurf(FH,VH,'CData',1*(JH>0),'EdgeColor','none',msoft{:},'FaceAlpha',0.9);
tsurf(FT,VT,'CData',ones(size(VT,1),1),'FaceAlpha',0.1,'EdgeColor','none',msoft{:});
hold off;
l = light('Position',1000*[-0.8 -0.8 2],'Style','infinite');
add_shadow(ts,l,'Color',0.9*[1 1 1],'Fade','infinite','Nudge',0.1);
axis equal;
view(-31,17);
colormap([1+0.2*(blue-1);blue;orange;])
camproj('persp');
camlight;
set(gca,'pos',[0 0 1 1])
set(gca,'Visible','off');set(gcf,'Color','w');
