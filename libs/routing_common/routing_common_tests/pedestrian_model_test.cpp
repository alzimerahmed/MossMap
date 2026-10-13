#include "testing/testing.hpp"

#include "routing_common/pedestrian_model.hpp"

#include "indexer/classificator.hpp"
#include "indexer/classificator_loader.hpp"
#include "indexer/feature_data.hpp"

#include "base/stl_helpers.hpp"

namespace pedestrian_model_test
{
using namespace routing;

class PedestrianModelTest
{
public:
  PedestrianModelTest() { classificator::Load(); }

  std::shared_ptr<VehicleModelInterface> GetModel()
  {
    return PedestrianModelFactory().GetVehicleModel();
  }

  SpeedParams DefaultSpeedParams() { return {kInvalidSpeed, true /* isCity */}; }
};

// Trail difficulty (sac_scale) is encoded by the generator as 3-level path types and
// should be weighted down so that hiking routes prefer easier trails.
UNIT_CLASS_TEST(PedestrianModelTest, PathGrade)
{
  auto const model = GetModel();
  TEST(model, ());
  auto const & cl = classif();

  auto const makeHolder = [&cl](base::StringIL const & path)
  {
    feature::TypesHolder holder;
    holder.Add(cl.GetTypeByPath(path));
    return holder;
  };

  auto const params = DefaultSpeedParams();
  auto const plainPath = model->GetSpeed(makeHolder({"highway", "path"}), params);
  auto const difficult = model->GetSpeed(makeHolder({"highway", "path", "difficult"}), params);
  auto const expert = model->GetSpeed(makeHolder({"highway", "path", "expert"}), params);

  TEST(plainPath.IsValid(), ());
  TEST(plainPath.m_weight > difficult.m_weight, (plainPath.m_weight, difficult.m_weight));
  TEST(plainPath.m_eta > difficult.m_eta, (plainPath.m_eta, difficult.m_eta));
  TEST(difficult.m_weight > expert.m_weight, (difficult.m_weight, expert.m_weight));
  TEST(difficult.m_eta > expert.m_eta, (difficult.m_eta, expert.m_eta));
}

// Surface quality affects the weight (route desirability) for pedestrian routing.
UNIT_CLASS_TEST(PedestrianModelTest, Surface)
{
  auto const model = GetModel();
  TEST(model, ());
  auto const & cl = classif();

  auto const makeHolder = [&cl](base::StringIL const & highway, base::StringIL const & surface)
  {
    feature::TypesHolder holder;
    holder.Add(cl.GetTypeByPath(highway));
    holder.Add(cl.GetTypeByPath(surface));
    return holder;
  };

  auto const params = DefaultSpeedParams();
  auto const pavedFootway = model->GetSpeed(makeHolder({"highway", "footway"}, {"psurface", "paved_good"}), params);
  auto const badTrail = model->GetSpeed(makeHolder({"highway", "path"}, {"psurface", "unpaved_bad"}), params);

  TEST(pavedFootway.IsValid(), ());
  TEST(pavedFootway.m_weight > badTrail.m_weight, (pavedFootway.m_weight, badTrail.m_weight));
}

}  // namespace pedestrian_model_test
