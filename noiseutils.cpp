/*  Copyright (c) 2013, Abdullah A. Hassan <voodooattack@hotmail.com>
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
 *
 *  1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
 *
 *  2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following
 *     disclaimer in the documentation and/or other materials provided with the distribution.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
 *  INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 *  IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
 *  OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
 *  OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 *  OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 */

#include "noiseutils.hpp"

#include "config.hpp"
#include "jsonutils.hpp"
#include "noisemodules.hpp"

#ifdef NOISE_DIR_IS_LIBNOISE
#include <libnoise/noise.h>
#else
#include <noise/noise.h>
#endif

#include <boost/algorithm/string.hpp>

namespace ADWIF
{
  std::shared_ptr<noise::module::Module> buildNoiseGraph(const Json::Value & value,
    std::vector<std::shared_ptr<noise::module::Module>> & modules,
    std::map<std::string, std::shared_ptr<noise::module::Module>> & defs,
    const std::shared_ptr<HeightMapModule> & heightMap, int seed)
  {
    std::shared_ptr<noise::module::Module> m;

    if (value["module"].empty())
    {
      if (!value["ref"].empty() && defs.find(value["ref"].asString()) != defs.end())
        return defs[value["ref"].asString()];
      else
        throw ParsingException("undefined module missing reference or reference not found:\n" +
                                value.toStyledString());
    }

    std::string module = value["module"].asString();
    boost::to_lower(module);

    if (module == "add")
    {
      std::shared_ptr<noise::module::Add> add(new noise::module::Add);
      std::shared_ptr<noise::module::Module> m1 = buildNoiseGraph(value["sources"][0], modules, defs, heightMap, seed);
      std::shared_ptr<noise::module::Module> m2 = buildNoiseGraph(value["sources"][1], modules, defs, heightMap, seed);
      add->SetSourceModule(0, *m1);
      add->SetSourceModule(1, *m2);
      m = add;
    }
    else if (module == "mul" || module == "multiply")
    {
      std::shared_ptr<noise::module::Multiply> mul(new noise::module::Multiply);
      std::shared_ptr<noise::module::Module> m1 = buildNoiseGraph(value["sources"][0], modules, defs, heightMap, seed);
      std::shared_ptr<noise::module::Module> m2 = buildNoiseGraph(value["sources"][1], modules, defs, heightMap, seed);
      mul->SetSourceModule(0, *m1);
      mul->SetSourceModule(1, *m2);
      m = mul;
    }
    else if (module == "pow" || module == "power")
    {
      std::shared_ptr<noise::module::Power> pow(new noise::module::Power);
      std::shared_ptr<noise::module::Module> m1 = buildNoiseGraph(value["sources"][0], modules, defs, heightMap, seed);
      std::shared_ptr<noise::module::Module> m2 = buildNoiseGraph(value["sources"][1], modules, defs, heightMap, seed);
      pow->SetSourceModule(0, *m1);
      pow->SetSourceModule(1, *m2);
      m = pow;
    }
    else if (module == "min")
    {
      std::shared_ptr<noise::module::Min> min(new noise::module::Min);
      std::shared_ptr<noise::module::Module> m1 = buildNoiseGraph(value["sources"][0], modules, defs, heightMap, seed);
      std::shared_ptr<noise::module::Module> m2 = buildNoiseGraph(value["sources"][1], modules, defs, heightMap, seed);
      min->SetSourceModule(0, *m1);
      min->SetSourceModule(1, *m2);
      m = min;
    }
    else if (module == "max")
    {
      std::shared_ptr<noise::module::Max> max(new noise::module::Max);
      std::shared_ptr<noise::module::Module> m1 = buildNoiseGraph(value["sources"][0], modules, defs, heightMap, seed);
      std::shared_ptr<noise::module::Module> m2 = buildNoiseGraph(value["sources"][1], modules, defs, heightMap, seed);
      max->SetSourceModule(0, *m1);
      max->SetSourceModule(1, *m2);
      m = max;
    }
    else if (module == "sel" || module == "select")
    {
      std::shared_ptr<noise::module::Select> sel(new noise::module::Select);
      std::shared_ptr<noise::module::Module> m1 = buildNoiseGraph(value["sources"][0], modules, defs, heightMap, seed);
      std::shared_ptr<noise::module::Module> m2 = buildNoiseGraph(value["sources"][1], modules, defs, heightMap, seed);
      std::shared_ptr<noise::module::Module> ctl;
      if (!value["controller"].empty())
        ctl = buildNoiseGraph(value["controller"], modules, defs, heightMap, seed);
      else
        ctl = buildNoiseGraph(value["sources"][2], modules, defs, heightMap, seed);
      sel->SetSourceModule(0, *m1);
      sel->SetSourceModule(1, *m2);
      sel->SetControlModule(*ctl);
      if (!value["falloff"].empty())
        sel->SetEdgeFalloff(value["falloff"].asDouble());
      if (value["bounds"].isArray())
      {
        if (value["bounds"][0].asDouble() < value["bounds"][1].asDouble())
          sel->SetBounds(value["bounds"][0].asDouble(), value["bounds"][1].asDouble());
        else
          throw ParsingException("Select: Upper bound must be greater than lower bound");
      }
      m = sel;
    }
    else if (module == "blend")
    {
      std::shared_ptr<noise::module::Blend> blend(new noise::module::Blend);
      std::shared_ptr<noise::module::Module> m1 = buildNoiseGraph(value["sources"][0], modules, defs, heightMap, seed);
      std::shared_ptr<noise::module::Module> m2 = buildNoiseGraph(value["sources"][1], modules, defs, heightMap, seed);
      std::shared_ptr<noise::module::Module> ctl;
      if (!value["controller"].empty())
        ctl = buildNoiseGraph(value["controller"], modules, defs, heightMap, seed);
      else
        ctl = buildNoiseGraph(value["sources"][2], modules, defs, heightMap, seed);
      blend->SetSourceModule(0, *m1);
      blend->SetSourceModule(1, *m2);
      blend->SetControlModule(*ctl);
      m = blend;
    }
    else if (module == "displace")
    {
      std::shared_ptr<noise::module::Blend> displace(new noise::module::Blend);
      std::shared_ptr<noise::module::Module> m1 = buildNoiseGraph(value["sources"][0], modules, defs, heightMap, seed);
      std::shared_ptr<noise::module::Module> m2 = buildNoiseGraph(value["sources"][1], modules, defs, heightMap, seed);
      std::shared_ptr<noise::module::Module> m3 = buildNoiseGraph(value["sources"][2], modules, defs, heightMap, seed);
      std::shared_ptr<noise::module::Module> m4 = buildNoiseGraph(value["sources"][3], modules, defs, heightMap, seed);
      displace->SetSourceModule(0, *m1);
      displace->SetSourceModule(1, *m2);
      displace->SetSourceModule(2, *m2);
      displace->SetSourceModule(3, *m3);
      m = displace;
    }
    else if (module == "curve")
    {
      std::shared_ptr<noise::module::Curve> curve(new noise::module::Curve);
      std::shared_ptr<noise::module::Module> src = buildNoiseGraph(value["sources"][0], modules, defs, heightMap, seed);
      curve->SetSourceModule(0, *src);
      for(const Json::Value & c : value["curve"])
        curve->AddControlPoint(c[0].asDouble(), c[1].asDouble()); // FIXME: No two control points can have the same input value.
      m = curve;
    }
    else if (module == "terrace")
    {
      std::shared_ptr<noise::module::Terrace> terrace(new noise::module::Terrace);
      std::shared_ptr<noise::module::Module> src = buildNoiseGraph(value["sources"][0], modules, defs, heightMap, seed);
      terrace->SetSourceModule(0, *src);
      for(const Json::Value & c : value["curve"])
        terrace->AddControlPoint(c.asDouble()); // FIXME: No two control points can have the same input value.
      m = terrace;
    }
    else if (module == "clamp")
    {
      std::shared_ptr<noise::module::Clamp> clamp(new noise::module::Clamp);
      std::shared_ptr<noise::module::Module> src = buildNoiseGraph(value["sources"][0], modules, defs, heightMap, seed);
      clamp->SetSourceModule(0, *src);
      clamp->SetBounds(value["min"].asDouble(), value["max"].asDouble());
      m = clamp;
    }
    else if (module == "exp" || module == "exponent")
    {
      std::shared_ptr<noise::module::Exponent> exp(new noise::module::Exponent);
      std::shared_ptr<noise::module::Module> src = buildNoiseGraph(value["sources"][0], modules, defs, heightMap, seed);
      exp->SetSourceModule(0, *src);
      if (!value["exponent"].empty())
        exp->SetExponent(value["exponent"].asDouble());
      m = exp;
    }
    else if (module == "abs")
    {
      std::shared_ptr<noise::module::Abs> abs(new noise::module::Abs);
      std::shared_ptr<noise::module::Module> src = buildNoiseGraph(value["sources"][0], modules, defs, heightMap, seed);
      abs->SetSourceModule(0, *src);
      m = abs;
    }
    else if (module == "invert")
    {
      std::shared_ptr<noise::module::Invert> invert(new noise::module::Invert);
      std::shared_ptr<noise::module::Module> src = buildNoiseGraph(value["sources"][0], modules, defs, heightMap, seed);
      invert->SetSourceModule(0, *src);
      m = invert;
    }
    else if (module == "cache")
    {
      std::shared_ptr<noise::module::Cache> cache(new noise::module::Cache);
      std::shared_ptr<noise::module::Module> src = buildNoiseGraph(value["sources"][0], modules, defs, heightMap, seed);
      cache->SetSourceModule(0, *src);
      m = cache;
    }
    else if (module == "scale" || module == "scalebias")
    {
      std::shared_ptr<noise::module::ScaleBias> scale(new noise::module::ScaleBias);
      std::shared_ptr<noise::module::Module> src = buildNoiseGraph(value["sources"][0], modules, defs, heightMap, seed);
      scale->SetSourceModule(0, *src);
      if (!value["scale"].empty())
        scale->SetScale(value["scale"].asDouble());
      if (!value["bias"].empty())
        scale->SetScale(value["bias"].asDouble());
      m = scale;
    }
    else if (module == "scalep" || module == "scalepoint")
    {
      std::shared_ptr<noise::module::ScalePoint> scale(new noise::module::ScalePoint);
      std::shared_ptr<noise::module::Module> src = buildNoiseGraph(value["sources"][0], modules, defs, heightMap, seed);
      scale->SetSourceModule(0, *src);
      if (!value["scale"].empty())
      {
        scale->SetScale(value["scale"].asDouble());
        if (!value["scalex"].empty())
          scale->SetXScale(value["scalex"].asDouble());
        if (!value["scaley"].empty())
          scale->SetYScale(value["scaley"].asDouble());
        if (!value["scalez"].empty())
          scale->SetZScale(value["scalez"].asDouble());
      }
      else if (value["scale"].isArray())
        scale->SetScale(value["scale"][0].asDouble(), value["scale"][1].asDouble(), value["scale"][2].asDouble());
      m = scale;
    }
    else if (module == "trans" || module == "translate")
    {
      std::shared_ptr<noise::module::TranslatePoint> trans(new noise::module::TranslatePoint);
      std::shared_ptr<noise::module::Module> src = buildNoiseGraph(value["sources"][0], modules, defs, heightMap, seed);
      trans->SetSourceModule(0, *src);
      if (!value["translation"].empty())
      {
        trans->SetTranslation(value["translation"].asDouble());
        if (!value["x"].empty())
          trans->SetXTranslation(value["x"].asDouble());
        if (!value["y"].empty())
          trans->SetYTranslation(value["y"].asDouble());
        if (!value["z"].empty())
          trans->SetZTranslation(value["z"].asDouble());
      }
      else if (value["translation"].isArray())
        trans->SetTranslation(value["translation"][0].asDouble(), value["translation"][1].asDouble(), value["translation"][2].asDouble());
      m = trans;
    }
    else if (module == "rot" || module == "rotate")
    {
      std::shared_ptr<noise::module::RotatePoint> rot(new noise::module::RotatePoint);
      std::shared_ptr<noise::module::Module> src = buildNoiseGraph(value["sources"][0], modules, defs, heightMap, seed);
      rot->SetSourceModule(0, *src);
      if (value["rotation"].isArray())
        rot->SetAngles(value["rotation"][0].asDouble(), value["rotation"][1].asDouble(), value["rotation"][2].asDouble());
      m = rot;
    }
    else if (module == "const" || module == "constant")
    {
      std::shared_ptr<noise::module::Const> con(new noise::module::Const);
      con->SetConstValue(value["value"].asDouble());
      m = con;
    }
    else if (module == "checkerboard")
    {
      std::shared_ptr<noise::module::Checkerboard> cb(new noise::module::Checkerboard);
      m = cb;
    }
    else if (module == "billow")
    {
      std::shared_ptr<noise::module::Billow> billow(new noise::module::Billow);
      if (!value["frequency"].empty())
        billow->SetFrequency(value["frequency"].asDouble());
      if (!value["lacunarity"].empty())
        billow->SetLacunarity(value["lacunarity"].asDouble());
      if (!value["octaves"].empty())
        billow->SetOctaveCount(value["octaves"].asInt());
      if (!value["persistence"].empty())
        billow->SetPersistence(value["persistence"].asDouble());
      if (value["quality"].isString())
      {
        std::string quality = value["quality"].asString();
        boost::to_lower(quality);
        if (quality == "fast")
          billow->SetNoiseQuality(noise::NoiseQuality::QUALITY_FAST);
        else if (quality == "standard")
          billow->SetNoiseQuality(noise::NoiseQuality::QUALITY_STD);
        else if (quality == "best")
          billow->SetNoiseQuality(noise::NoiseQuality::QUALITY_BEST);
        else
          throw ParsingException("unsupported quality specifier:\n" + value.toStyledString());
      }
      int s = seed;
      if (!value["seed"].empty())
        s ^= value["seed"].asInt();
      billow->SetSeed(seed);
      m = billow;
    }
    else if (module == "perlin")
    {
      std::shared_ptr<noise::module::Perlin> perlin(new noise::module::Perlin);
      if (!value["frequency"].empty())
        perlin->SetFrequency(value["frequency"].asDouble());
      if (!value["lacunarity"].empty())
        perlin->SetLacunarity(value["lacunarity"].asDouble());
      if (!value["octaves"].empty())
        perlin->SetOctaveCount(value["octaves"].asInt());
      if (!value["persistence"].empty())
        perlin->SetPersistence(value["persistence"].asDouble());
      if (value["quality"].isString())
      {
        std::string quality = value["quality"].asString();
        boost::to_lower(quality);
        if (quality == "fast")
          perlin->SetNoiseQuality(noise::NoiseQuality::QUALITY_FAST);
        else if (quality == "standard")
          perlin->SetNoiseQuality(noise::NoiseQuality::QUALITY_STD);
        else if (quality == "best")
          perlin->SetNoiseQuality(noise::NoiseQuality::QUALITY_BEST);
        else
          throw ParsingException("unsupported quality specifier:\n" + value.toStyledString());
      }
      int s = seed;
      if (!value["seed"].empty())
        s ^= value["seed"].asInt();
      perlin->SetSeed(seed);
      m = perlin;
    }
    else if (module == "ridged" || module == "ridgedmulti")
    {
      std::shared_ptr<noise::module::RidgedMulti> ridged(new noise::module::RidgedMulti);
      if (!value["frequency"].empty())
        ridged->SetFrequency(value["frequency"].asDouble());
      if (!value["lacunarity"].empty())
        ridged->SetLacunarity(value["lacunarity"].asDouble());
      if (!value["octaves"].empty())
        ridged->SetOctaveCount(value["octaves"].asInt());
      if (value["quality"].isString())
      {
        std::string quality = value["quality"].asString();
        boost::to_lower(quality);
        if (quality == "fast")
          ridged->SetNoiseQuality(noise::NoiseQuality::QUALITY_FAST);
        else if (quality == "standard")
          ridged->SetNoiseQuality(noise::NoiseQuality::QUALITY_STD);
        else if (quality == "best")
          ridged->SetNoiseQuality(noise::NoiseQuality::QUALITY_BEST);
        else
          throw ParsingException("unsupported quality specifier:\n" + value.toStyledString());
      }
      int s = seed;
      if (!value["seed"].empty())
        s ^= value["seed"].asInt();
      ridged->SetSeed(seed);
      m = ridged;
    }
    else if (module == "voronoi")
    {
      std::shared_ptr<noise::module::Voronoi> voronoi(new noise::module::Voronoi);
      if (!value["frequency"].empty())
        voronoi->SetFrequency(value["frequency"].asDouble());
      if (!value["displacement"].empty())
        voronoi->SetDisplacement(value["displacement"].asDouble());
      int s = seed;
      if (!value["seed"].empty())
        s ^= value["seed"].asInt();
      voronoi->SetSeed(seed);
      m = voronoi;
    }
    else if (module == "cylinders")
    {
      std::shared_ptr<noise::module::Cylinders> cylinders(new noise::module::Cylinders);
      if (!value["frequency"].empty())
        cylinders->SetFrequency(value["frequency"].asDouble());
      m = cylinders;
    }
    else if (module == "spheres")
    {
      std::shared_ptr<noise::module::Spheres> spheres(new noise::module::Spheres);
      if (!value["frequency"].empty())
        spheres->SetFrequency(value["frequency"].asDouble());
      m = spheres;
    }
    else if (module == "turbulence")
    {
      std::shared_ptr<noise::module::Turbulence> turbulence(new noise::module::Turbulence);
      std::shared_ptr<noise::module::Module> src = buildNoiseGraph(value["sources"][0], modules, defs, heightMap, seed);
      turbulence->SetSourceModule(0, *src);
      if (!value["frequency"].empty())
        turbulence->SetFrequency(value["frequency"].asDouble());
      if (!value["power"].empty())
        turbulence->SetPower(value["power"].asDouble());
      if (!value["roughness"].empty())
        turbulence->SetRoughness(value["roughness"].asDouble());
      int s = seed;
      if (!value["seed"].empty())
        s ^= value["seed"].asInt();
      turbulence->SetSeed(seed);
      m = turbulence;
    }
    else if (module == "heightmap")
    {
      m = heightMap;
    }
    else
      throw ParsingException("unknown noise module:\n" + value.toStyledString());

    modules.push_back(m);

    if (value["name"].isString())
      defs.insert({value["name"].asString(), m});

    return m;
  }
}
