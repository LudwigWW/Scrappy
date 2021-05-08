[VA,FA] = load_mesh('../examples/busts/Karl-40K.ply');
VB = VA;
FB = FA;
%model = [
%      0.8233095    0.02231794  7.200244e-08      17.46022
%    -0.02231794     0.8233095 -7.200244e-08      23.89763
%   -7.39271e-08  7.002491e-08     0.8236119        20.994
%              0             0             0             1
%];
%p = [
%  112.3445 118.5552 121.8481
%];
%np = [
%  8.385402e-08   -0.9591781   -0.2828026
%];
%s = 0.823612;
model = [
      0.7549159  6.599684e-08   -0.05201119      33.83025
  -7.070026e-08     0.7567055 -6.599684e-08      28.71106
     0.05201119  7.070026e-08     0.7549159       24.1836
              0             0             0             1
];
p = [115 111 128];
np = [0.03 -0.09 -1];
s = 0.756705;

VBT = VB*model(1:3,1:3)' + model(1:3,4)';

[VL,FL,JL,VR,FR,JR] = matryoshka(VA,FA,VBT,FB,p,np);
AL = ambient_occlusion(VL,FL,barycenter(VL,FL),normals(VL,FL),1000);
AR = ambient_occlusion(VR,FR,barycenter(VR,FR),normals(VR,FR),1000);
%save('karl.mat');
save('karl-hat.mat');

pink = [254 194 194]/255;
teal = [144 216 196]/255;
gold = [1 0.85 0];
CM = interp1([0 1],[teal;1-0.6*(1-teal)],linspace(0,1,10));
matryoshka_video( ...
  VL, FL, JL, ...
  VR, FR, JR, ...
  model, np,  ...
  'AL',AL, ...
  'AR',AR, ...
  'Callback',@() figgif('karl.gif'), ...
  ... 'Callback',@() vidObj.writeVideo(myaa('raw')), ...
  'ColorMap',CM, ...
  'BackgroundColor',teal, ...
  'ShadowColor',0.9*teal, ...
  'SliceColor',repmat([pink;gold],size(CM,1)/2,1), ...
  'Levels',size(CM,1), ...
  'FramesPerLevel',10, ...
  'NumFaces',size(FA,1), ...
  'LightParams', {'Position',[1 -1 10],'Style','infinite'}, ...
  'Rotation',axisangle2matrix([0 0 1],-pi*0.25), ...
  'View',[-38 10], ...
  'ViewChange',[65 -10]);
% Read an animation
[X,M] = imread('karl.gif');
% Convert to raw color image sequence
Y = cell2mat(permute(arrayfun(@(C) ...
ind2rgb( ...
X(:,:,:,C),M(:,:,C)),1:size(X,4),'UniformOutput',false),[1 4 3 2]));
% Trim animation to fit
C = imtrim(Y);
% Write back to animated .gif
imwrite_gif(C(:,:,:,[1:end end-1:-1:1]),'karl.gif');

