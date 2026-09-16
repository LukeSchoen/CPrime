struct BaseForwardVector
{
  double x, y, z;
  BaseForwardVector(double xValue, double yValue, double zValue)
    : x(xValue), y(yValue), z(zValue) {}
};

class BaseForwardTarget
{
public:
  BaseForwardVector position;
  BaseForwardVector rotation;
  double fov, aspect, nearPlane, farPlane;

  BaseForwardTarget(const BaseForwardVector &pos,
                    const BaseForwardVector &rot, double fovValue,
                    double aspectValue, double nearValue, double farValue);
};

class DerivedForwardTarget : public BaseForwardTarget
{
public:
  DerivedForwardTarget(const BaseForwardVector &pos,
                       const BaseForwardVector &rot, double fovValue,
                       double aspectValue, double nearValue = 0.1,
                       double farValue = 1000.0);
};

BaseForwardTarget::BaseForwardTarget(const BaseForwardVector &pos,
                                     const BaseForwardVector &rot,
                                     double fovValue, double aspectValue,
                                     double nearValue, double farValue)
  : position(pos), rotation(rot), fov(fovValue), aspect(aspectValue),
    nearPlane(nearValue), farPlane(farValue) {}

DerivedForwardTarget::DerivedForwardTarget(const BaseForwardVector &pos,
                                           const BaseForwardVector &rot,
                                           double fovValue, double aspectValue,
                                           double nearValue, double farValue)
  : BaseForwardTarget(pos, rot, fovValue, aspectValue, nearValue, farValue) {}

int main()
{
  DerivedForwardTarget target(BaseForwardVector(1.0, 2.0, 3.0),
                              { 4.0, 5.0, 6.0 },
                              45.0, 1.75);
  return target.position.x == 1.0 && target.position.y == 2.0
      && target.position.z == 3.0 && target.rotation.x == 4.0
      && target.rotation.y == 5.0 && target.rotation.z == 6.0
      && target.fov == 45.0 && target.aspect == 1.75
      && target.nearPlane == 0.1 && target.farPlane == 1000.0 ? 0 : 1;
}
