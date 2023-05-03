nbr=$1;shift
apiKey="KEY0185D52CEEF26CBA888211D708D87BE3_mVine39rFf32gIzvOr1uz1"

curl -X GET \
    --header "Content-Type: application/json" \
  --header "Accept: application/json" \
  --header "Authorization: Bearer ${apiKey}" \
  "https://api.telnyx.com/v2/number_lookup/+1${nbr}?type=carrier&type=caller-name"
