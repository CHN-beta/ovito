////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2016 OVITO GmbH, Germany
//
//  This file is part of OVITO (Open Visualization Tool).
//
//  OVITO is free software; you can redistribute it and/or modify it either under the
//  terms of the GNU General Public License version 3 as published by the Free Software
//  Foundation (the "GPL") or, at your option, under the terms of the MIT License.
//  If you do not alter this notice, a recipient may use your version of this
//  file under either the GPL or the MIT License.
//
//  You should have received a copy of the GPL along with this program in a
//  file LICENSE.GPL.txt.  You should have received a copy of the MIT License along
//  with this program in a file LICENSE.MIT.txt
//
//  This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY KIND,
//  either express or implied. See the GPL or the MIT License for the specific language
//  governing rights and limitations.
//
////////////////////////////////////////////////////////////////////////////////////////

#pragma once


namespace Ovito { namespace StdMod {

// Viridis colormap by Nathaniel J. Smith, Stefan van der Walt and Eric Firing
// Colormap data released under the CC0 license / public domain dedication.
// See https://bids.github.io/colormap/ for details
const float colormap_viridis_data[][3] = {
	{0.267004f, 0.004874f, 0.329415f},
	{0.268510f, 0.009605f, 0.335427f},
	{0.269944f, 0.014625f, 0.341379f},
	{0.271305f, 0.019942f, 0.347269f},
	{0.272594f, 0.025563f, 0.353093f},
	{0.273809f, 0.031497f, 0.358853f},
	{0.274952f, 0.037752f, 0.364543f},
	{0.276022f, 0.044167f, 0.370164f},
	{0.277018f, 0.050344f, 0.375715f},
	{0.277941f, 0.056324f, 0.381191f},
	{0.278791f, 0.062145f, 0.386592f},
	{0.279566f, 0.067836f, 0.391917f},
	{0.280267f, 0.073417f, 0.397163f},
	{0.280894f, 0.078907f, 0.402329f},
	{0.281446f, 0.084320f, 0.407414f},
	{0.281924f, 0.089666f, 0.412415f},
	{0.282327f, 0.094955f, 0.417331f},
	{0.282656f, 0.100196f, 0.422160f},
	{0.282910f, 0.105393f, 0.426902f},
	{0.283091f, 0.110553f, 0.431554f},
	{0.283197f, 0.115680f, 0.436115f},
	{0.283229f, 0.120777f, 0.440584f},
	{0.283187f, 0.125848f, 0.444960f},
	{0.283072f, 0.130895f, 0.449241f},
	{0.282884f, 0.135920f, 0.453427f},
	{0.282623f, 0.140926f, 0.457517f},
	{0.282290f, 0.145912f, 0.461510f},
	{0.281887f, 0.150881f, 0.465405f},
	{0.281412f, 0.155834f, 0.469201f},
	{0.280868f, 0.160771f, 0.472899f},
	{0.280255f, 0.165693f, 0.476498f},
	{0.279574f, 0.170599f, 0.479997f},
	{0.278826f, 0.175490f, 0.483397f},
	{0.278012f, 0.180367f, 0.486697f},
	{0.277134f, 0.185228f, 0.489898f},
	{0.276194f, 0.190074f, 0.493001f},
	{0.275191f, 0.194905f, 0.496005f},
	{0.274128f, 0.199721f, 0.498911f},
	{0.273006f, 0.204520f, 0.501721f},
	{0.271828f, 0.209303f, 0.504434f},
	{0.270595f, 0.214069f, 0.507052f},
	{0.269308f, 0.218818f, 0.509577f},
	{0.267968f, 0.223549f, 0.512008f},
	{0.266580f, 0.228262f, 0.514349f},
	{0.265145f, 0.232956f, 0.516599f},
	{0.263663f, 0.237631f, 0.518762f},
	{0.262138f, 0.242286f, 0.520837f},
	{0.260571f, 0.246922f, 0.522828f},
	{0.258965f, 0.251537f, 0.524736f},
	{0.257322f, 0.256130f, 0.526563f},
	{0.255645f, 0.260703f, 0.528312f},
	{0.253935f, 0.265254f, 0.529983f},
	{0.252194f, 0.269783f, 0.531579f},
	{0.250425f, 0.274290f, 0.533103f},
	{0.248629f, 0.278775f, 0.534556f},
	{0.246811f, 0.283237f, 0.535941f},
	{0.244972f, 0.287675f, 0.537260f},
	{0.243113f, 0.292092f, 0.538516f},
	{0.241237f, 0.296485f, 0.539709f},
	{0.239346f, 0.300855f, 0.540844f},
	{0.237441f, 0.305202f, 0.541921f},
	{0.235526f, 0.309527f, 0.542944f},
	{0.233603f, 0.313828f, 0.543914f},
	{0.231674f, 0.318106f, 0.544834f},
	{0.229739f, 0.322361f, 0.545706f},
	{0.227802f, 0.326594f, 0.546532f},
	{0.225863f, 0.330805f, 0.547314f},
	{0.223925f, 0.334994f, 0.548053f},
	{0.221989f, 0.339161f, 0.548752f},
	{0.220057f, 0.343307f, 0.549413f},
	{0.218130f, 0.347432f, 0.550038f},
	{0.216210f, 0.351535f, 0.550627f},
	{0.214298f, 0.355619f, 0.551184f},
	{0.212395f, 0.359683f, 0.551710f},
	{0.210503f, 0.363727f, 0.552206f},
	{0.208623f, 0.367752f, 0.552675f},
	{0.206756f, 0.371758f, 0.553117f},
	{0.204903f, 0.375746f, 0.553533f},
	{0.203063f, 0.379716f, 0.553925f},
	{0.201239f, 0.383670f, 0.554294f},
	{0.199430f, 0.387607f, 0.554642f},
	{0.197636f, 0.391528f, 0.554969f},
	{0.195860f, 0.395433f, 0.555276f},
	{0.194100f, 0.399323f, 0.555565f},
	{0.192357f, 0.403199f, 0.555836f},
	{0.190631f, 0.407061f, 0.556089f},
	{0.188923f, 0.410910f, 0.556326f},
	{0.187231f, 0.414746f, 0.556547f},
	{0.185556f, 0.418570f, 0.556753f},
	{0.183898f, 0.422383f, 0.556944f},
	{0.182256f, 0.426184f, 0.557120f},
	{0.180629f, 0.429975f, 0.557282f},
	{0.179019f, 0.433756f, 0.557430f},
	{0.177423f, 0.437527f, 0.557565f},
	{0.175841f, 0.441290f, 0.557685f},
	{0.174274f, 0.445044f, 0.557792f},
	{0.172719f, 0.448791f, 0.557885f},
	{0.171176f, 0.452530f, 0.557965f},
	{0.169646f, 0.456262f, 0.558030f},
	{0.168126f, 0.459988f, 0.558082f},
	{0.166617f, 0.463708f, 0.558119f},
	{0.165117f, 0.467423f, 0.558141f},
	{0.163625f, 0.471133f, 0.558148f},
	{0.162142f, 0.474838f, 0.558140f},
	{0.160665f, 0.478540f, 0.558115f},
	{0.159194f, 0.482237f, 0.558073f},
	{0.157729f, 0.485932f, 0.558013f},
	{0.156270f, 0.489624f, 0.557936f},
	{0.154815f, 0.493313f, 0.557840f},
	{0.153364f, 0.497000f, 0.557724f},
	{0.151918f, 0.500685f, 0.557587f},
	{0.150476f, 0.504369f, 0.557430f},
	{0.149039f, 0.508051f, 0.557250f},
	{0.147607f, 0.511733f, 0.557049f},
	{0.146180f, 0.515413f, 0.556823f},
	{0.144759f, 0.519093f, 0.556572f},
	{0.143343f, 0.522773f, 0.556295f},
	{0.141935f, 0.526453f, 0.555991f},
	{0.140536f, 0.530132f, 0.555659f},
	{0.139147f, 0.533812f, 0.555298f},
	{0.137770f, 0.537492f, 0.554906f},
	{0.136408f, 0.541173f, 0.554483f},
	{0.135066f, 0.544853f, 0.554029f},
	{0.133743f, 0.548535f, 0.553541f},
	{0.132444f, 0.552216f, 0.553018f},
	{0.131172f, 0.555899f, 0.552459f},
	{0.129933f, 0.559582f, 0.551864f},
	{0.128729f, 0.563265f, 0.551229f},
	{0.127568f, 0.566949f, 0.550556f},
	{0.126453f, 0.570633f, 0.549841f},
	{0.125394f, 0.574318f, 0.549086f},
	{0.124395f, 0.578002f, 0.548287f},
	{0.123463f, 0.581687f, 0.547445f},
	{0.122606f, 0.585371f, 0.546557f},
	{0.121831f, 0.589055f, 0.545623f},
	{0.121148f, 0.592739f, 0.544641f},
	{0.120565f, 0.596422f, 0.543611f},
	{0.120092f, 0.600104f, 0.542530f},
	{0.119738f, 0.603785f, 0.541400f},
	{0.119512f, 0.607464f, 0.540218f},
	{0.119423f, 0.611141f, 0.538982f},
	{0.119483f, 0.614817f, 0.537692f},
	{0.119699f, 0.618490f, 0.536347f},
	{0.120081f, 0.622161f, 0.534946f},
	{0.120638f, 0.625828f, 0.533488f},
	{0.121380f, 0.629492f, 0.531973f},
	{0.122312f, 0.633153f, 0.530398f},
	{0.123444f, 0.636809f, 0.528763f},
	{0.124780f, 0.640461f, 0.527068f},
	{0.126326f, 0.644107f, 0.525311f},
	{0.128087f, 0.647749f, 0.523491f},
	{0.130067f, 0.651384f, 0.521608f},
	{0.132268f, 0.655014f, 0.519661f},
	{0.134692f, 0.658636f, 0.517649f},
	{0.137339f, 0.662252f, 0.515571f},
	{0.140210f, 0.665859f, 0.513427f},
	{0.143303f, 0.669459f, 0.511215f},
	{0.146616f, 0.673050f, 0.508936f},
	{0.150148f, 0.676631f, 0.506589f},
	{0.153894f, 0.680203f, 0.504172f},
	{0.157851f, 0.683765f, 0.501686f},
	{0.162016f, 0.687316f, 0.499129f},
	{0.166383f, 0.690856f, 0.496502f},
	{0.170948f, 0.694384f, 0.493803f},
	{0.175707f, 0.697900f, 0.491033f},
	{0.180653f, 0.701402f, 0.488189f},
	{0.185783f, 0.704891f, 0.485273f},
	{0.191090f, 0.708366f, 0.482284f},
	{0.196571f, 0.711827f, 0.479221f},
	{0.202219f, 0.715272f, 0.476084f},
	{0.208030f, 0.718701f, 0.472873f},
	{0.214000f, 0.722114f, 0.469588f},
	{0.220124f, 0.725509f, 0.466226f},
	{0.226397f, 0.728888f, 0.462789f},
	{0.232815f, 0.732247f, 0.459277f},
	{0.239374f, 0.735588f, 0.455688f},
	{0.246070f, 0.738910f, 0.452024f},
	{0.252899f, 0.742211f, 0.448284f},
	{0.259857f, 0.745492f, 0.444467f},
	{0.266941f, 0.748751f, 0.440573f},
	{0.274149f, 0.751988f, 0.436601f},
	{0.281477f, 0.755203f, 0.432552f},
	{0.288921f, 0.758394f, 0.428426f},
	{0.296479f, 0.761561f, 0.424223f},
	{0.304148f, 0.764704f, 0.419943f},
	{0.311925f, 0.767822f, 0.415586f},
	{0.319809f, 0.770914f, 0.411152f},
	{0.327796f, 0.773980f, 0.406640f},
	{0.335885f, 0.777018f, 0.402049f},
	{0.344074f, 0.780029f, 0.397381f},
	{0.352360f, 0.783011f, 0.392636f},
	{0.360741f, 0.785964f, 0.387814f},
	{0.369214f, 0.788888f, 0.382914f},
	{0.377779f, 0.791781f, 0.377939f},
	{0.386433f, 0.794644f, 0.372886f},
	{0.395174f, 0.797475f, 0.367757f},
	{0.404001f, 0.800275f, 0.362552f},
	{0.412913f, 0.803041f, 0.357269f},
	{0.421908f, 0.805774f, 0.351910f},
	{0.430983f, 0.808473f, 0.346476f},
	{0.440137f, 0.811138f, 0.340967f},
	{0.449368f, 0.813768f, 0.335384f},
	{0.458674f, 0.816363f, 0.329727f},
	{0.468053f, 0.818921f, 0.323998f},
	{0.477504f, 0.821444f, 0.318195f},
	{0.487026f, 0.823929f, 0.312321f},
	{0.496615f, 0.826376f, 0.306377f},
	{0.506271f, 0.828786f, 0.300362f},
	{0.515992f, 0.831158f, 0.294279f},
	{0.525776f, 0.833491f, 0.288127f},
	{0.535621f, 0.835785f, 0.281908f},
	{0.545524f, 0.838039f, 0.275626f},
	{0.555484f, 0.840254f, 0.269281f},
	{0.565498f, 0.842430f, 0.262877f},
	{0.575563f, 0.844566f, 0.256415f},
	{0.585678f, 0.846661f, 0.249897f},
	{0.595839f, 0.848717f, 0.243329f},
	{0.606045f, 0.850733f, 0.236712f},
	{0.616293f, 0.852709f, 0.230052f},
	{0.626579f, 0.854645f, 0.223353f},
	{0.636902f, 0.856542f, 0.216620f},
	{0.647257f, 0.858400f, 0.209861f},
	{0.657642f, 0.860219f, 0.203082f},
	{0.668054f, 0.861999f, 0.196293f},
	{0.678489f, 0.863742f, 0.189503f},
	{0.688944f, 0.865448f, 0.182725f},
	{0.699415f, 0.867117f, 0.175971f},
	{0.709898f, 0.868751f, 0.169257f},
	{0.720391f, 0.870350f, 0.162603f},
	{0.730889f, 0.871916f, 0.156029f},
	{0.741388f, 0.873449f, 0.149561f},
	{0.751884f, 0.874951f, 0.143228f},
	{0.762373f, 0.876424f, 0.137064f},
	{0.772852f, 0.877868f, 0.131109f},
	{0.783315f, 0.879285f, 0.125405f},
	{0.793760f, 0.880678f, 0.120005f},
	{0.804182f, 0.882046f, 0.114965f},
	{0.814576f, 0.883393f, 0.110347f},
	{0.824940f, 0.884720f, 0.106217f},
	{0.835270f, 0.886029f, 0.102646f},
	{0.845561f, 0.887322f, 0.099702f},
	{0.855810f, 0.888601f, 0.097452f},
	{0.866013f, 0.889868f, 0.095953f},
	{0.876168f, 0.891125f, 0.095250f},
	{0.886271f, 0.892374f, 0.095374f},
	{0.896320f, 0.893616f, 0.096335f},
	{0.906311f, 0.894855f, 0.098125f},
	{0.916242f, 0.896091f, 0.100717f},
	{0.926106f, 0.897330f, 0.104071f},
	{0.935904f, 0.898570f, 0.108131f},
	{0.945636f, 0.899815f, 0.112838f},
	{0.955300f, 0.901065f, 0.118128f},
	{0.964894f, 0.902323f, 0.123941f},
	{0.974417f, 0.903590f, 0.130215f},
	{0.983868f, 0.904867f, 0.136897f},
	{0.993248f, 0.906157f, 0.143936f}
};

// Magma colormap by Nathaniel J. Smith and Stefan van der Walt
// Colormap data released under the CC0 license / public domain dedication.
// See https://bids.github.io/colormap/ for details
const float colormap_magma_data[][3] = {
	{0.001462f, 0.000466f, 0.013866f},
	{0.002258f, 0.001295f, 0.018331f},
	{0.003279f, 0.002305f, 0.023708f},
	{0.004512f, 0.003490f, 0.029965f},
	{0.005950f, 0.004843f, 0.037130f},
	{0.007588f, 0.006356f, 0.044973f},
	{0.009426f, 0.008022f, 0.052844f},
	{0.011465f, 0.009828f, 0.060750f},
	{0.013708f, 0.011771f, 0.068667f},
	{0.016156f, 0.013840f, 0.076603f},
	{0.018815f, 0.016026f, 0.084584f},
	{0.021692f, 0.018320f, 0.092610f},
	{0.024792f, 0.020715f, 0.100676f},
	{0.028123f, 0.023201f, 0.108787f},
	{0.031696f, 0.025765f, 0.116965f},
	{0.035520f, 0.028397f, 0.125209f},
	{0.039608f, 0.031090f, 0.133515f},
	{0.043830f, 0.033830f, 0.141886f},
	{0.048062f, 0.036607f, 0.150327f},
	{0.052320f, 0.039407f, 0.158841f},
	{0.056615f, 0.042160f, 0.167446f},
	{0.060949f, 0.044794f, 0.176129f},
	{0.065330f, 0.047318f, 0.184892f},
	{0.069764f, 0.049726f, 0.193735f},
	{0.074257f, 0.052017f, 0.202660f},
	{0.078815f, 0.054184f, 0.211667f},
	{0.083446f, 0.056225f, 0.220755f},
	{0.088155f, 0.058133f, 0.229922f},
	{0.092949f, 0.059904f, 0.239164f},
	{0.097833f, 0.061531f, 0.248477f},
	{0.102815f, 0.063010f, 0.257854f},
	{0.107899f, 0.064335f, 0.267289f},
	{0.113094f, 0.065492f, 0.276784f},
	{0.118405f, 0.066479f, 0.286321f},
	{0.123833f, 0.067295f, 0.295879f},
	{0.129380f, 0.067935f, 0.305443f},
	{0.135053f, 0.068391f, 0.315000f},
	{0.140858f, 0.068654f, 0.324538f},
	{0.146785f, 0.068738f, 0.334011f},
	{0.152839f, 0.068637f, 0.343404f},
	{0.159018f, 0.068354f, 0.352688f},
	{0.165308f, 0.067911f, 0.361816f},
	{0.171713f, 0.067305f, 0.370771f},
	{0.178212f, 0.066576f, 0.379497f},
	{0.184801f, 0.065732f, 0.387973f},
	{0.191460f, 0.064818f, 0.396152f},
	{0.198177f, 0.063862f, 0.404009f},
	{0.204935f, 0.062907f, 0.411514f},
	{0.211718f, 0.061992f, 0.418647f},
	{0.218512f, 0.061158f, 0.425392f},
	{0.225302f, 0.060445f, 0.431742f},
	{0.232077f, 0.059889f, 0.437695f},
	{0.238826f, 0.059517f, 0.443256f},
	{0.245543f, 0.059352f, 0.448436f},
	{0.252220f, 0.059415f, 0.453248f},
	{0.258857f, 0.059706f, 0.457710f},
	{0.265447f, 0.060237f, 0.461840f},
	{0.271994f, 0.060994f, 0.465660f},
	{0.278493f, 0.061978f, 0.469190f},
	{0.284951f, 0.063168f, 0.472451f},
	{0.291366f, 0.064553f, 0.475462f},
	{0.297740f, 0.066117f, 0.478243f},
	{0.304081f, 0.067835f, 0.480812f},
	{0.310382f, 0.069702f, 0.483186f},
	{0.316654f, 0.071690f, 0.485380f},
	{0.322899f, 0.073782f, 0.487408f},
	{0.329114f, 0.075972f, 0.489287f},
	{0.335308f, 0.078236f, 0.491024f},
	{0.341482f, 0.080564f, 0.492631f},
	{0.347636f, 0.082946f, 0.494121f},
	{0.353773f, 0.085373f, 0.495501f},
	{0.359898f, 0.087831f, 0.496778f},
	{0.366012f, 0.090314f, 0.497960f},
	{0.372116f, 0.092816f, 0.499053f},
	{0.378211f, 0.095332f, 0.500067f},
	{0.384299f, 0.097855f, 0.501002f},
	{0.390384f, 0.100379f, 0.501864f},
	{0.396467f, 0.102902f, 0.502658f},
	{0.402548f, 0.105420f, 0.503386f},
	{0.408629f, 0.107930f, 0.504052f},
	{0.414709f, 0.110431f, 0.504662f},
	{0.420791f, 0.112920f, 0.505215f},
	{0.426877f, 0.115395f, 0.505714f},
	{0.432967f, 0.117855f, 0.506160f},
	{0.439062f, 0.120298f, 0.506555f},
	{0.445163f, 0.122724f, 0.506901f},
	{0.451271f, 0.125132f, 0.507198f},
	{0.457386f, 0.127522f, 0.507448f},
	{0.463508f, 0.129893f, 0.507652f},
	{0.469640f, 0.132245f, 0.507809f},
	{0.475780f, 0.134577f, 0.507921f},
	{0.481929f, 0.136891f, 0.507989f},
	{0.488088f, 0.139186f, 0.508011f},
	{0.494258f, 0.141462f, 0.507988f},
	{0.500438f, 0.143719f, 0.507920f},
	{0.506629f, 0.145958f, 0.507806f},
	{0.512831f, 0.148179f, 0.507648f},
	{0.519045f, 0.150383f, 0.507443f},
	{0.525270f, 0.152569f, 0.507192f},
	{0.531507f, 0.154739f, 0.506895f},
	{0.537755f, 0.156894f, 0.506551f},
	{0.544015f, 0.159033f, 0.506159f},
	{0.550287f, 0.161158f, 0.505719f},
	{0.556571f, 0.163269f, 0.505230f},
	{0.562866f, 0.165368f, 0.504692f},
	{0.569172f, 0.167454f, 0.504105f},
	{0.575490f, 0.169530f, 0.503466f},
	{0.581819f, 0.171596f, 0.502777f},
	{0.588158f, 0.173652f, 0.502035f},
	{0.594508f, 0.175701f, 0.501241f},
	{0.600868f, 0.177743f, 0.500394f},
	{0.607238f, 0.179779f, 0.499492f},
	{0.613617f, 0.181811f, 0.498536f},
	{0.620005f, 0.183840f, 0.497524f},
	{0.626401f, 0.185867f, 0.496456f},
	{0.632805f, 0.187893f, 0.495332f},
	{0.639216f, 0.189921f, 0.494150f},
	{0.645633f, 0.191952f, 0.492910f},
	{0.652056f, 0.193986f, 0.491611f},
	{0.658483f, 0.196027f, 0.490253f},
	{0.664915f, 0.198075f, 0.488836f},
	{0.671349f, 0.200133f, 0.487358f},
	{0.677786f, 0.202203f, 0.485819f},
	{0.684224f, 0.204286f, 0.484219f},
	{0.690661f, 0.206384f, 0.482558f},
	{0.697098f, 0.208501f, 0.480835f},
	{0.703532f, 0.210638f, 0.479049f},
	{0.709962f, 0.212797f, 0.477201f},
	{0.716387f, 0.214982f, 0.475290f},
	{0.722805f, 0.217194f, 0.473316f},
	{0.729216f, 0.219437f, 0.471279f},
	{0.735616f, 0.221713f, 0.469180f},
	{0.742004f, 0.224025f, 0.467018f},
	{0.748378f, 0.226377f, 0.464794f},
	{0.754737f, 0.228772f, 0.462509f},
	{0.761077f, 0.231214f, 0.460162f},
	{0.767398f, 0.233705f, 0.457755f},
	{0.773695f, 0.236249f, 0.455289f},
	{0.779968f, 0.238851f, 0.452765f},
	{0.786212f, 0.241514f, 0.450184f},
	{0.792427f, 0.244242f, 0.447543f},
	{0.798608f, 0.247040f, 0.444848f},
	{0.804752f, 0.249911f, 0.442102f},
	{0.810855f, 0.252861f, 0.439305f},
	{0.816914f, 0.255895f, 0.436461f},
	{0.822926f, 0.259016f, 0.433573f},
	{0.828886f, 0.262229f, 0.430644f},
	{0.834791f, 0.265540f, 0.427671f},
	{0.840636f, 0.268953f, 0.424666f},
	{0.846416f, 0.272473f, 0.421631f},
	{0.852126f, 0.276106f, 0.418573f},
	{0.857763f, 0.279857f, 0.415496f},
	{0.863320f, 0.283729f, 0.412403f},
	{0.868793f, 0.287728f, 0.409303f},
	{0.874176f, 0.291859f, 0.406205f},
	{0.879464f, 0.296125f, 0.403118f},
	{0.884651f, 0.300530f, 0.400047f},
	{0.889731f, 0.305079f, 0.397002f},
	{0.894700f, 0.309773f, 0.393995f},
	{0.899552f, 0.314616f, 0.391037f},
	{0.904281f, 0.319610f, 0.388137f},
	{0.908884f, 0.324755f, 0.385308f},
	{0.913354f, 0.330052f, 0.382563f},
	{0.917689f, 0.335500f, 0.379915f},
	{0.921884f, 0.341098f, 0.377376f},
	{0.925937f, 0.346844f, 0.374959f},
	{0.929845f, 0.352734f, 0.372677f},
	{0.933606f, 0.358764f, 0.370541f},
	{0.937221f, 0.364929f, 0.368567f},
	{0.940687f, 0.371224f, 0.366762f},
	{0.944006f, 0.377643f, 0.365136f},
	{0.947180f, 0.384178f, 0.363701f},
	{0.950210f, 0.390820f, 0.362468f},
	{0.953099f, 0.397563f, 0.361438f},
	{0.955849f, 0.404400f, 0.360619f},
	{0.958464f, 0.411324f, 0.360014f},
	{0.960949f, 0.418323f, 0.359630f},
	{0.963310f, 0.425390f, 0.359469f},
	{0.965549f, 0.432519f, 0.359529f},
	{0.967671f, 0.439703f, 0.359810f},
	{0.969680f, 0.446936f, 0.360311f},
	{0.971582f, 0.454210f, 0.361030f},
	{0.973381f, 0.461520f, 0.361965f},
	{0.975082f, 0.468861f, 0.363111f},
	{0.976690f, 0.476226f, 0.364466f},
	{0.978210f, 0.483612f, 0.366025f},
	{0.979645f, 0.491014f, 0.367783f},
	{0.981000f, 0.498428f, 0.369734f},
	{0.982279f, 0.505851f, 0.371874f},
	{0.983485f, 0.513280f, 0.374198f},
	{0.984622f, 0.520713f, 0.376698f},
	{0.985693f, 0.528148f, 0.379371f},
	{0.986700f, 0.535582f, 0.382210f},
	{0.987646f, 0.543015f, 0.385210f},
	{0.988533f, 0.550446f, 0.388365f},
	{0.989363f, 0.557873f, 0.391671f},
	{0.990138f, 0.565296f, 0.395122f},
	{0.990871f, 0.572706f, 0.398714f},
	{0.991558f, 0.580107f, 0.402441f},
	{0.992196f, 0.587502f, 0.406299f},
	{0.992785f, 0.594891f, 0.410283f},
	{0.993326f, 0.602275f, 0.414390f},
	{0.993834f, 0.609644f, 0.418613f},
	{0.994309f, 0.616999f, 0.422950f},
	{0.994738f, 0.624350f, 0.427397f},
	{0.995122f, 0.631696f, 0.431951f},
	{0.995480f, 0.639027f, 0.436607f},
	{0.995810f, 0.646344f, 0.441361f},
	{0.996096f, 0.653659f, 0.446213f},
	{0.996341f, 0.660969f, 0.451160f},
	{0.996580f, 0.668256f, 0.456192f},
	{0.996775f, 0.675541f, 0.461314f},
	{0.996925f, 0.682828f, 0.466526f},
	{0.997077f, 0.690088f, 0.471811f},
	{0.997186f, 0.697349f, 0.477182f},
	{0.997254f, 0.704611f, 0.482635f},
	{0.997325f, 0.711848f, 0.488154f},
	{0.997351f, 0.719089f, 0.493755f},
	{0.997351f, 0.726324f, 0.499428f},
	{0.997341f, 0.733545f, 0.505167f},
	{0.997285f, 0.740772f, 0.510983f},
	{0.997228f, 0.747981f, 0.516859f},
	{0.997138f, 0.755190f, 0.522806f},
	{0.997019f, 0.762398f, 0.528821f},
	{0.996898f, 0.769591f, 0.534892f},
	{0.996727f, 0.776795f, 0.541039f},
	{0.996571f, 0.783977f, 0.547233f},
	{0.996369f, 0.791167f, 0.553499f},
	{0.996162f, 0.798348f, 0.559820f},
	{0.995932f, 0.805527f, 0.566202f},
	{0.995680f, 0.812706f, 0.572645f},
	{0.995424f, 0.819875f, 0.579140f},
	{0.995131f, 0.827052f, 0.585701f},
	{0.994851f, 0.834213f, 0.592307f},
	{0.994524f, 0.841387f, 0.598983f},
	{0.994222f, 0.848540f, 0.605696f},
	{0.993866f, 0.855711f, 0.612482f},
	{0.993545f, 0.862859f, 0.619299f},
	{0.993170f, 0.870024f, 0.626189f},
	{0.992831f, 0.877168f, 0.633109f},
	{0.992440f, 0.884330f, 0.640099f},
	{0.992089f, 0.891470f, 0.647116f},
	{0.991688f, 0.898627f, 0.654202f},
	{0.991332f, 0.905763f, 0.661309f},
	{0.990930f, 0.912915f, 0.668481f},
	{0.990570f, 0.920049f, 0.675675f},
	{0.990175f, 0.927196f, 0.682926f},
	{0.989815f, 0.934329f, 0.690198f},
	{0.989434f, 0.941470f, 0.697519f},
	{0.989077f, 0.948604f, 0.704863f},
	{0.988717f, 0.955742f, 0.712242f},
	{0.988367f, 0.962878f, 0.719649f},
	{0.988033f, 0.970012f, 0.727077f},
	{0.987691f, 0.977154f, 0.734536f},
	{0.987387f, 0.984288f, 0.742002f},
	{0.987053f, 0.991438f, 0.749504f}
};

}	// End of namespace
}	// End of namespace
