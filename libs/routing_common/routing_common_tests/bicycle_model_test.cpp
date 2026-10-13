#include "testing/testing.hpp"

#include "routing_common/bicycle_model.hpp"

#include "indexer/classificator.hpp"
#include "indexer/classificator_loader.hpp"
#include "indexer/feature_data.hpp"

#include "base/stl_helpers.hpp"

namespace bicycle_model_test
{
using namespace routing;

class BicycleModelTest
{
public:
  BicycleModelTest() { classificator::Load(); }

  std::shared_ptr<VehicleModel> GetModel(std::string const & country)
  {
    return std::dynamic_pointer_cast<VehicleModel>(BicycleModelFactory().GetVehicleModelForCountry(country));
  }

  SpeedParams DefaultSpeedParams() { return {kInvalidSpeed, true /* isCity */}; }
};

UNIT_CLASS_TEST(BicycleModelTest, Turkey)
{
  auto const model = GetModel("Turkey");
  TEST(model, ());
  auto const & cl = classif();

  feature::TypesHolder holder;
  holder.Add(cl.GetTypeByPath({"highway", "footway", "tunnel"}));

  TEST(model->HasRoadType(holder), ());
  TEST_EQUAL(model->GetSpeed(holder, DefaultSpeedParams()), BicycleModel::DismountSpeed(), ());
}

// Trail difficulty (sac_scale) is encoded by the generator as 3-level path types and
// should be weighted down in the default (non-MTB) bicycle profile.
UNIT_CLASS_TEST(BicycleModelTest, PathGrade)
{
  auto const model = GetModel("");
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

}  // namespace bicycle_model_test
