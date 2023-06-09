#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>


typedef struct Api_t
{
	int		n;
	char	*tag;
} Api_t ;


static Api_t ApiTable[] =
{
	{0, "Demo Mode Flag"},
	{1, "ProtoBase Transaction Type"},
	{2, "Transaction Amount"},
	{3, "Account number"},
	{4, "Expiration date"},
	{5, "Site ID"},
	{6, "Authorization code"},
	{7, "Reference number"},
	{8, "Re-Select Flag"},
	{9, "Host Batch number"},
	{10, "Risk Wizard"},
	{11, "User Defined Field"},
	{12, "Force flag"},
	{13, "Transaction Date"},
	{14, "Transaction Time"},
	{15, "Softrans Internal"},
	{16, "Unused"},
	{17, "Cash Back"},
	{18, "DEBIT PIN Block"},
	{19, "DEBIT Key Serial Number"},
	{20, "DEBIT Surcharge Amount"},
	{21, "DEBIT Network ID"},
	{22, "DEBIT Authentication Code"},
	{23, "Debit Account Type"},
	{24, "Debit Capable Flag"},
	{25, "Business Date"},
	{27, "DEBIT Network Name"},
	{29, "Batch Control Flag"},
	{30, "Online Indicator"},
	{31, "Commercial  Card Type"},
	{32, "Authorization Transaction Date"},
	{33, "Authorization Transaction Time"},
	{34, "Authorization Characteristics Indicator"},
	{35, "Validation Code"},
	{36, "Transaction Identifier"},
	{37, "Authorizer"},
	{38, "Reason Code"},
	{39, "Compliance Data"},
	{40, "Compliance Data"},
	{41, "Compliance Data"},
	{42, "Compliance Data"},
	{43, "System Trace Audit Number"},
	{44, "Expansion Field 1"},
	{45, "Expansion Field 2"},
	{46, "Expansion Field 3"},
	{47, "POS Data Code "},
	{48, "Expansion Field 5"},
	{49, "Card Product Result Code"},
	{50, "CVV2/CVC2/CID Information"},
	{51, "Effective Date"},
	{52, "Transponder / Proximity "},
	{53, "POS entry characteristics"},
	{60, "Resume Key"},
	{61, "Search Key"},
	{70, "Customer Code"},
	{71, "Tax 1 Indicator"},
	{72, "Tax Amount 1"},
	{73, "Tax 2 Indicator"},
	{74, "Tax Amount 2"},
	{75, "Tax 3 Indicator"},
	{76, "Tax Amount 3"},
	{77, "Tax 4 Indicator"},
	{78, "Tax Amount 4"},
	{79, "Tax 1 Type"},
	{80, "Tax 1 Rate"},
	{81, "Tax 2 Type"},
	{82, "Tax 2 Rate"},
	{83, "Tax 3 Type"},
	{84, "Tax 3 Rate"},
	{85, "Tax 4 Type"},
	{86, "Tax 4 Rate"},
	{90, "Report Type"},
	{91, "Detail Type"},
	{92, "Sort Order"},
	{93, "History month/year"},
	{94, "Report Page Width"},
	{95, "Report Page Length"},
	{100, "X-coordinate"},
	{101, "Y-coordinate"},
	{102, "Foreground color"},
	{103, "Background color"},
	{104, "Box type"},
	{105, "Data files path"},
	{106, "Batch file path"},
	{107, "Report file path"},
	{108, "History file path"},
	{109, "Terminal ID"},
	{110, "Cashier ID"},
	{111, "Upload level"},
	{112, "Processor ID"},
	{113, "Upload Criteria"},
	{114, "Host Report File Path"},
	{115, "Transaction qualifier"},
	{116, "Offline flag"},
	{117, "Stored Value Function"},
	{118, "Reversal flag"},
	{119, "EBT Qualifier"},
	{120, "Signature Capture Data Path"},
	{121, "Signature Capture Data Length"},
	{122, "Signature Capture  Code"},
	{123, "Signature Capture Status"},
	{124, "Unused"},
	{125, "Retrieval Reference Number"},
	{126, "Track Indicator"},
	{127, "Compliance Data"},
	{128, "Original Authorization Amount"},
	{129, "Compliance Data"},
	{130, "Total Authorized Amount"},
	{131, "CPS Total Incremental Auths sent"},
	{132, "CPS Authorization Reversal Sent"},
	{140, "Domestic Currency Trigraph"},
	{141, "Domestic Currency Code"},
	{142, "Target Currency Trigraph"},
	{143, "Target Currency Code"},
	{144, "Converted Currency Amount"},
	{145, "Rounding Method"},
	{146, "Target Currency Longname"},
	{147, "Target Currency Symbol"},
	{148, "Domestic Country"},
	{149, "Domestic Language"},
	{150, "DCC Conversion Rate"},
	{151, "Conversion Flag"},
	{152, "Source Terminal Name/Address"},
	{153, "Conversion Date"},
	{154, "Conversion Time"},
	{155, "Receipt Print Flag"},
	{156, "Merchants Currency Symbol"},
	{157, "Terminal Query Flag"},
	{158, "Merchant Fee"},
	{159, "DCC Conversion Rate Exponent"},
	{159, "Frequent Shopper Table ID"},
	{160, "DCC Total Authorized Amount "},
	{160, "Frequent Shopper Tender"},
	{161, "DC Replacement Terminal"},
	{162, "DCC Original Authorized Amount"},
	{170, "Document Type"},
	{171, "Text Form Path"},
	{172, "Form Type"},
	{173, "Binary Form Path"},
	{174, "Form Format "},
	{190, "eCommerce Indicator"},
	{191, "Electronic Goods Indicator "},
	{192, "Cardholder Certificate"},
	{193, "Merchant Certificate"},
	{194, "CAVV / AVV / Tran Stain "},
	{195, "XID"},
	{196, "CAVV Response"},
	{200, "Charge description"},
	{201, "Tip 1 amount"},
	{202, "Tip 2 amount"},
	{203, "Emp Number 1"},
	{204, "Emp number 2"},
	{205, "Check Number (ROC)"},
	{206, "Tax amount"},
	{207, "Food amount"},
	{208, "Beverage amount"},
	{209, "Table Number"},
	{210, "Description of Tip #1"},
	{211, "Adjusted Tip Amount"},
	{212, "Available Field (formerly ROC)"},
	{213, "Food Description"},
	{214, "Beverage Description"},
	{215, "Description of Tip #2"},
	{216, "Number of Guests"},
	{300, "Charge description"},
	{301, "Folio number"},
	{302, "Room rate"},
	{303, "Arrival date"},
	{304, "Departure date"},
	{305, "Program code"},
	{306, "Room Tax"},
	{307, "Duration of stay"},
	{308, "Ticket number/ ROC #"},
	{309, "Employee number"},
	{310, "Room Number"},
	{311, "Extra Charges Amount"},
	{312, "Extra Charges Reason Codes"},
	{313, "Guest Name"},
	{320, "Prepaid Expenses"},
	{321, "Food/Beverage Charges"},
	{322, "Folio/CashAdvance"},
	{323, "Valet Parking Charges"},
	{324, "Mini-Bar Charges"},
	{325, "Laundry Charges"},
	{326, "Telephone Charges"},
	{327, "Gift Shop Charges"},
	{328, "Movie Charges"},
	{329, "Business Center Charges"},
	{330, "Health Club Charges"},
	{331, "Other Charges"},
	{400, "Product Code"},
	{401, "Invoice number / ROC"},
	{402, "Unused"},
	{403, "Unused"},
	{404, "Unused"},
	{405, "Unused"},
	{406, "Item 1 text"},
	{407, "Item 2 text"},
	{408, "Item 3 text"},
	{409, "Item 4 text"},
	{410, "Item 5 text"},
	{411, "Retail Terms"},
	{412, "Item Code 1"},
	{413, "Item Code 2"},
	{414, "Item Code 3"},
	{415, "Item Code 4"},
	{416, "Item Code 5"},
	{430, "Shipped to zip code"},
	{431, "Ship From Postal Code"},
	{432, "Freight Amount"},
	{433, "Discount Amount"},
	{434, "Duty / Handling Amount"},
	{435, "Country Code"},
	{436, "Purchase Identifier"},
	{437, "Summary Commodity Code"},
	{438, "VAT Invoice Number"},
	{439, "Merchant Code"},
	{440, "Item Product Code"},
	{441, "Item Description"},
	{442, "Item Unit Amount"},
	{443, "Item Quanity"},
	{444, "Item Extended Amount"},
	{445, "Item Tax Amount"},
	{446, "Item Tax Rate Applied"},
	{447, "Item Tax Type"},
	{448, "Item Type of Supply"},
	{449, "Item Unit Measure"},
	{450, "Item Commidity Code"},
	{451, "Customer Tax Number"},
	{452, "Merchant Tax Number"},
	{453, "Item Discount Amount"},
	{454, "PO Line Number"},
	{500, "Vehicle number"},
	{501, "Extra Charges Amount"},
	{502, "Rental date"},
	{503, "Rental time"},
	{504, "Rental city"},
	{505, "Rental state"},
	{506, "Return date"},
	{507, "Return time"},
	{508, "Return city"},
	{509, "Return state"},
	{510, "Renter Name"},
	{511, "Rental agreement"},
	{512, "Length of rental"},
	{513, "Promotion code"},
	{514, "Certificate number"},
	{515, "Reservation city"},
	{516, "Reservation state"},
	{517, "Unused"},
	{518, "Rental Return Country"},
	{519, "Program Code"},
	{520, "Return location ID"},
	{521, "Extra Charges Reason Code"},
	{529, "Daily Rental Rate"},
	{530, "Weekly Rental Rate"},
	{531, "Insurance Charge"},
	{532, "Fuel Charge"},
	{533, "One Way Charges"},
	{534, "Renter Name"},
	{535, "Auto Towing Charge"},
	{536, "Mileage Charges"},
	{537, "Extra Milage Charges"},
	{538, "Late Return Charges"},
	{539, "Return Location"},
	{540, "Telephone Charges"},
	{541, "Total Tax / VAT"},
	{542, "Other Charges"},
	{543, "Corporate ID Code"},
	{544, "Car Class Code"},
	{600, "Invoice Number"},
	{601, "Product Code 1"},
	{602, "Product Code 2"},
	{603, "Product Code 3"},
	{604, "Product Code 4"},
	{605, "Product Code 5"},
	{606, "Product Code 6"},
	{607, "Product Code 7"},
	{608, "Product Amount 1"},
	{609, "Product Amount 2"},
	{610, "Product Amount 3"},
	{611, "Product Amount 4"},
	{612, "Product Amount 5"},
	{613, "Product Amount 6"},
	{614, "Product Amount 7"},
	{615, "Promotion Code"},
	{616, "Promotion Start Date"},
	{617, "Cash Price"},
	{618, "Credit Plan"},
	{619, "Tax Amount"},
	{620, "Down Payment"},
	{621, "Deferred Payment"},
	{622, "Deferred Indicator"},
	{623, "Deferred Date"},
	{624, "Extended Period"},
	{625, "Free Period"},
	{626, "Single Coverage"},
	{627, "Joint Coverage"},
	{628, "Salesperson"},
	{629, "Sales Amount"},
	{630, "Promotion End Date"},
	{631, "Open To Buy Amount"},
	{632, "Account Balance"},
	{633, "Last Payment Amount"},
	{634, "Last Payment Date"},
	{635, "Next Payment Due Amount"},
	{636, "Next Payment Due Date"},
	{637, "Sales Order Number"},
	{638, "Product SKU Number"},
	{639, "Product SKU Number"},
	{640, "Product SKU Number"},
	{641, "Product SKU Number"},
	{642, "Product SKU Number"},
	{643, "Product SKU Number"},
	{644, "Product SKU Number"},
	{645, "Previous Balance "},
	{646, "TotalInvAmount"},
	{647, "Partial Authorization Acceptance"},
	{675, "Chain Name"},
	{676, "Charge Description"},
	{677, "ROC #"},
	{678, "Tax Amount"},
	{679, "Gallons "},
	{680, "Price/Gallon"},
	{681, "Product Code 1"},
	{682, "Dollar Amount 1"},
	{683, "Product Code 2"},
	{684, "Dollar Amount 2"},
	{685, "Product Code 3"},
	{686, "Dollar amount 3"},
	{687, "Vehicle Number"},
	{688, "Odometer Reading"},
	{689, "Driver Number"},
	{700, "Billing Zip Code"},
	{701, "Billing Street Address"},
	{702, "Cardholder first name"},
	{703, "Cardholder last name"},
	{704, "Billing House Number"},
	{705, "Billing Street Name"},
	{706, "Billing Apartment Number"},
	{707, "Billing City"},
	{708, "Billing State"},
	{709, "Billing Phone Number"},
	{710, "Phone Number Type"},
	{711, "Shipping Date"},
	{712, "Charge Description"},
	{713, "Clearing Sequence Count"},
	{714, "Clearing Sequence Number"},
	{715, "Order Number"},
	{716, "Ship to Zip Code"},
	{717, "Item 1 text"},
	{718, "Item 2 text"},
	{719, "Item 3 text"},
	{720, "Item Code 1"},
	{721, "Item Code 2"},
	{722, "Item Code 3"},
	{723, "Unused"},
	{724, "Ship to Address Line 1"},
	{725, "Ship to Address Line 2"},
	{726, "Ship to City"},
	{727, "Ship to State"},
	{728, "Cardholder Certificate"},
	{732, "Electronic Goods Indicator "},
	{750, "Ext. payment code"},
	{752, "Charge description"},
	{753, "ROC #"},
	{755, "Ticket number"},
	{775, "Policy number"},
	{776, "ROC #"},
	{777, "Policy Type"},
	{778, "Coverage Period"},
	{779, "Coverage Dates"},
	{780, "Charge Type"},
	{800, "ROC "},
	{801, "Originating Phone Number"},
	{802, "Destination Phone Number"},
	{803, "Carrier ID"},
	{804, "Time of Call"},
	{805, "Duration of Call"},
	{806, "Rate Class"},
	{807, "Originating City"},
	{808, "Originating State"},
	{809, "Destination City"},
	{810, "Destination State"},
	{811, "Foreign Exchange"},
	{900, "Account number / MICR"},
	{901, "Driver's license"},
	{902, "Check number"},
	{903, "State ID"},
	{904, "State code"},
	{905, "Date of birth"},
	{906, "Check type"},
	{907, "MICR Entry Mode"},
	{908, "Driver's License Entry Mode"},
	{909, "Manager Override"},
	{910, "Zip Code"},
	{911, "Phone Number"},
	{912, "ABA Number"},
	{913, "Alternate Identification Indicator"},
	{914, "Alternate Identification"},
	{915, "Coupon Number"},
	{916, "Check Status"},
	{917, "ACH Reference Number"},
	{918, "Returned Check Note"},
	{919, "Returned Check Fee"},
	{920, "Password"},
	{921, "Last name"},
	{922, "First name"},
	{923, "SS number"},
	{924, "supervisor ID"},
	{925, "Supervisor password"},
	{926, "User disable reason code"},
	{927, "New password"},
	{928, "Decline Record Number"},
	{950, "Ticket indicator"},
	{951, "Clearing seq. number"},
	{952, "Clearing seq. count"},
	{953, "Ticket number"},
	{954, "Passenger Name"},
	{955, "Departure date"},
	{956, "Org. city / airport code"},
	{957, "Extended Code Indicator"},
	{958, "Carrier Name"},
	{959, "Issuing City"},
	{960, "Number in party"},
	{961, "Document Type"},
	{962, "Document Numbers"},
	{963, "Carrier Code 1"},
	{964, "Service Class 1"},
	{965, "Stop over code 1"},
	{966, "Destination City/Airport Code 1"},
	{967, "Carrier Code 2"},
	{968, "Service Class 2"},
	{969, "Stop over code 2"},
	{970, "Destination City/Airport Code 2"},
	{971, "Carrier Code 3"},
	{972, "Service Class 3"},
	{973, "Stop over code 3"},
	{974, "Destination City/Airport Code 3"},
	{975, "Carrier Code 4"},
	{976, "Service Class 4"},
	{977, "Stop over code 4"},
	{978, "Destination City/Airport Code 4"},
	{1000, "Card Type"},
	{1001, "Card name"},
	{1002, "Cardholder name"},
	{1003, "PB response code"},
	{1004, "Host response msg"},
	{1005, "Merchant number"},
	{1006, "Terminal number"},
	{1007, "Debit working Key"},
	{1007, "Merchant Key"},
	{1008, "Masked Account Number"},
	{1009, "Host response code"},
	{1010, "ProtoBase response"},
	{1011, "Host reference number"},
	{1012, "ProtoBase Batch Number"},
	{1013, "Local Batch Net Amount"},
	{1014, "Local Batch Tran Count"},
	{1015, "AVS result code"},
	{1016, "Host Batch Net Amount"},
	{1017, "Host Batch Tran Count"},
	{1018, "Funded Batch Amount"},
	{1019, "Funded Batch Count"},
	{1020, "TranDate"},
	{1021, "TranTime"},
	{1022, "MSR Query"},
	{1023, "Frequent Shopper Balance"},
	{1024, "Frequent Shopper Prompt"},
	{1025, "Frequent Shopper Validation"},
	{1026, "Frequency Points"},
	{1026, "Frequent Shopper Points"},
	{1027, "Disclosure"},
	{1028, "Frequency Points Base"},
	{1029, "Frequency Points Bonus"},
	{1030, "Frequency Points Redemption"},
	{1035, "Totals Number of Cards"},
	{1036, "Sequence Number"},
	{1046, "Trace Number"},
	{1047, "Transaction Code"},
	{1048, "Canadian Resp Code"},
	{1049, "MAC Key"},
	{1050, "Tracing number"},
	{1051, "Time of Authorization"},
	{1060, "Voucher Number"},
	{1061, "Number of  Accounts"},
	{1062, "Account 1 Balance Details"},
	{1063, "Account 2 Balance Details"},
	{1064, "Account 3 Balance Details"},
	{1065, "Account 4 Balance Details"},
	{1066, "Account 5 Balance Details"},
	{1067, "Account 6 Balance Details"},
	{1068, "Account 7 Balance Details"},
	{1069, "Account 8 Balance Details"},
	{1070, "Account 9 Balance Details"},
	{1071, "Account 10 Balance Details "},
	{1072, "Balance Information Capable Flag"},
	{1073, "Number of Request Accounts"},
	{1074, "Request Account 1 Balance Details"},
	{1075, "Request Account 2 Balance Details"},
	{1076, "Request Account 3 Balance Details"},
	{1077, "Request Account 4 Balance Details"},
	{1078, "Request Account 5 Balance Details"},
	{1079, "Request Account 6 Balance Details"},
	{1080, "Request Account 7 Balance Details"},
	{1081, "Request Account 8 Balance Details"},
	{1082, "Request Account 9 Balance Details"},
	{1083, "Request Account 10 Balance Details "},
	{1100, "Batch resp. Msg."},
	{1101, "User Defined Field"},
	{1102, "User Defined Field"},
	{1103, "User Defined Field"},
	{1104, "User Defined Field"},
	{1105, "User Defined Field"},
	{1106, "Batch Assignment Name"},
	{2000, "Private Label Card Total Count"},
	{2001, "Private Label Card Total Amount"},
	{2002, "Visa  Total Count"},
	{2003, "Visa Total Amount"},
	{2004, "Mastercard Total Count"},
	{2005, "Mastercard Total Amount"},
	{2006, "Other Credit Card Total Count"},
	{2007, "Other Credit Card Total Amount"},
	{2008, "Total Credit Sales Count"},
	{2009, "Total Credit Sales Amount"},
	{2010, "Total Debit Card Sales Count"},
	{2011, "Total Debit Card Sales Amount"},
	{2012, "Total Credit Card Void Count"},
	{2013, "Total Credit Card Void Amount"},
	{2014, "Total Debit Card Void Count"},
	{2015, "Total Debit Card Void Amount"},
	{2016, "Total Credit Card Return Count"},
	{2017, "Total Credit Card Return Amount"},
	{2018, "Total Debit Card Return Count"},
	{2019, "Total Debit Card Return Amount"},
	{2020, "Amex Card count"},
	{2021, "Amex Card Amount "},
	{2022, "Discover Card count"},
	{2023, "Discover Amount"},
	{2024, "Private Label Card Total"},
	{2025, "Private  Label Amount"},
	{2100, "LID Tax 1 Indicator"},
	{2101, "LID Tax 1 Type"},
	{2102, "LID Tax 1 Amount"},
	{2103, "LID Tax 1 Rate"},
	{2104, "LID Tax 2 Indicator"},
	{2105, "LID Tax 2 Type"},
	{2106, "LID Tax 2 Amount"},
	{2107, "LID Tax 2 Rate"},
	{2108, "LID Tax 3 Indicator"},
	{2109, "LID Tax 3 Type"},
	{2110, "LID Tax 3 Amount"},
	{2111, "LID Tax 3 Rate"},
	{2112, "Requester Name"},
	{2113, "Requesters ID Code"},
	{2114, "Requester Address "},
	{2115, "Requester City"},
	{2116, "Requester State Code"},
	{2117, "Requester Zip Code"},
	{2118, "Requester Country"},
	{2119, "Requester Phone"},
	{2120, "Requester Email"},
	{2121, "Buying Group Name"},
	{2122, "Buying Group Address "},
	{2123, "Buying Group City"},
	{2124, "Buying Group State Code"},
	{2125, "Buying Group Zip Code"},
	{2126, "Buying Group Country"},
	{2127, "Buying Group Phone"},
	{2128, "Buying Group Email"},
	{2129, "Shipped From Name "},
	{2130, "SF Address "},
	{2131, "SF City"},
	{2132, "SF State Code"},
	{2133, "SF Country"},
	{2134, "SF Phone"},
	{2135, "SF Email"},
	{2136, "Item List Price"},
	{2137, "Basis Code for Unit Price"},
	{2138, "Manufacturer Part #"},
	{2139, "Supplier Catalog #"},
	{2140, "Supplier Stock Unit #"},
	{2141, "Vendor Part #"},
	{2142, "Client Defined Asset #"},
	{2143, "Item S&H Discount Amount"},
	{2144, "Item S&H Discount Rate"},
	{2145, "Item Freight Amount"},
	{2146, "Item Freight Rate"},
	{2147, "Item Handling Amount"},
	{2148, "Item Handling Rate"},
	{2149, "Carrier Alpha Code"},
	{2150, "Bill of Lading Number"},
	{2151, "Shipment Number"},
	{2152, "Item Net Weight"},
	{2153, "Item Net Weight Basis"},
	{2154, "Item Billed Weight"},
	{2155, "Item Billed Weight Basis"},
	{2156, "Quantity Delivered"},
	{2157, "Back Ordered"},
	{2158, "Out of Stock"},
	{2159, "Partial Shipment"},
	{2160, "Split Shipment"},
	{2161, "Lease Event Date"},
	{2162, "Lease Service Date"},
	{2163, "Lease Actual End Date"},
	{2164, "Lease Planned End Date"},
	{2165, "Lease Order Date"},
	{2166, "Reference ID #1"},
	{2167, "Reference ID Value #1"},
	{2168, "Reference Description #1"},
	{2169, "Reference ID #2"},
	{2170, "Reference ID Value #2"},
	{2171, "Reference Description #2"},
	{2172, "Reference ID #3"},
	{2173, "Reference ID Value #3"},
	{2174, "Reference Description #3"},
	{2175, "Reference ID #4"},
	{2176, "Reference ID Value #4"},
	{2177, "Reference Description #4"},
	{2178, "Reference ID #5"},
	{2179, "Reference ID Value #5"},
	{2180, "Reference Description #5"},
	{2190, "Customer Hostname"},
	{2191, "Http Browser Type"},
	{2192, "Customer IP"},
	{2193, "Customer II Digits"},
	{2194, "Shipping Method"},
	{2195, "Ship-To First Name"},
	{2196, "Ship-To Last Name"},
	{2197, "Ship-To Phone Number"},
	{2198, "Ship-To Country Code"},
	{3000, "Title"},
	{3001, "Name Prefix"},
	{3002, "First Name"},
	{3003, "Middle Initial"},
	{3004, "Middle Name"},
	{3005, "Last Name"},
	{3006, "Secondary Name"},
	{3007, "Suffix"},
	{3008, "Date of Birth"},
	{3009, "Age"},
	{3010, "Social Security Number"},
	{3011, "Marital Status"},
	{3012, "Dependants"},
	{3013, "Social Security Override Flag"},
	{3014, "Other Phone Number"},
	{3018, "Address Line 1"},
	{3019, "Address Line 2"},
	{3020, "Street Number"},
	{3021, "Street Name"},
	{3022, "Street Type"},
	{3023, "Apartment Number"},
	{3024, "PO Box"},
	{3025, "Rural Route Number"},
	{3026, "City"},
	{3027, "State"},
	{3028, "Zip Code"},
	{3029, "Home Phone Number"},
	{3030, "Home Phone ext"},
	{3031, "Home Phone Status"},
	{3032, "Time At Address"},
	{3033, "Housing Indicator"},
	{3034, "Payment Type"},
	{3035, "Mortgage/Rent Payment"},
	{3036, "Home Value"},
	{3037, "Mortgage Company/Landlord"},
	{3038, "Balance of 1st Mortgage"},
	{3039, "Email Adress"},
	{3040, "Time at Address"},
	{3041, "Street Number"},
	{3042, "Street Name"},
	{3043, "Apartment Number"},
	{3044, "PO Box"},
	{3045, "Rural Route Number"},
	{3046, "City"},
	{3047, "State"},
	{3048, "Zip Code"},
	{3049, "Address Line 1"},
	{3050, "Address Line 2"},
	{3060, "Retired Indicator"},
	{3061, "Self Emplyed Indicator"},
	{3062, "Occupation Code"},
	{3063, "Employer Name"},
	{3064, "Employer City"},
	{3065, "Employer State"},
	{3066, "Time at Employer"},
	{3067, "Employer Phone Number"},
	{3068, "Home Phone ext"},
	{3069, "Net Monthly Income"},
	{3070, "Yearly Income"},
	{3080, "Employer"},
	{3081, "Employer City"},
	{3082, "Employer State"},
	{3083, "Time at Employer"},
	{3084, "Employer Phone Number"},
	{3085, "Position"},
	{3090, "Other Monthly Income"},
	{3091, "Other Income Source"},
	{3092, "Checking Account Indicator"},
	{3093, "Savings Account Indicator"},
	{3094, "Application Card #"},
	{3095, "Application Card Type"},
	{3096, "VAR Application ID"},
	{3100, "Title"},
	{3101, "Prefix"},
	{3102, "First Name"},
	{3103, "Middle Initial"},
	{3104, "Middle Name"},
	{3105, "Last Name"},
	{3106, "Secondary Last Name"},
	{3107, "Suffix"},
	{3108, "Date of Birth"},
	{3109, "Social Security Number"},
	{3110, "Street Number"},
	{3111, "Street Name"},
	{3112, "Apartment Number"},
	{3113, "PO Box"},
	{3114, "Rural Route Number"},
	{3115, "City"},
	{3116, "State"},
	{3117, "Zip Code"},
	{3118, "Yearly Income"},
	{3119, "Address Line 1"},
	{3120, "Address Line 2"},
	{3121, "Social Security Override Flag"},
	{3122, "Phone Number"},
	{3123, "Occupation"},
	{3124, "Other Income"},
	{3125, "Other Phone Number"},
	{3126, "Housing Indicator"},
	{3127, "Time at Address"},
	{3128, "Other Income Source"},
	{3130, "Occupation Code"},
	{3131, "Employer"},
	{3132, "Employer City"},
	{3133, "Employer State"},
	{3134, "Time at Employer"},
	{3135, "Phone Number"},
	{3136, "Net Monthly Income"},
	{3137, "Joint ID Number"},
	{3138, "Joint ID State"},
	{3139, "Joint ID Expiration Date"},
	{3140, "Relative Indicator"},
	{3141, "Reference Name"},
	{3142, "Reference Address"},
	{3143, "Reference City"},
	{3144, "Reference State"},
	{3145, "Reference Zip Code"},
	{3146, "Reference Phone"},
	{3147, "City"},
	{3148, "State"},
	{3149, "Zip Code"},
	{3150, "Address Line 1"},
	{3151, "Address Line 2"},
	{3152, "Time at Address"},
	{3153, "Employer"},
	{3154, "Employer City"},
	{3155, "Employer State"},
	{3156, "Phone Number"},
	{3157, "Time at Employer"},
	{3158, "Mortgage/Rent Payment"},
	{3159, "Total Bank Cards"},
	{3160, "Total Asset Amount"},
	{3161, "Alternate Product Indicator"},
	{3162, "Closing Branch"},
	{3163, "Chargeguard Insurance"},
	{3164, "Insurance Code"},
	{3165, "Application Number"},
	{3166, "Sales Person"},
	{3167, "User Id"},
	{3168, "Help Desk Phone Number"},
	{3169, "Score Number"},
	{3170, "Requested Product Code"},
	{3171, "Down Payment"},
	{3172, "Credit Type"},
	{3173, "Number of Cards"},
	{3174, "Optional Data Line 1"},
	{3175, "Optional Data Line 2"},
	{3176, "Pass Date Days"},
	{3177, "Requested Credit Line"},
	{3178, "Pre approved number"},
	{3179, "PL Account Number"},
	{3180, "Pass Expire Date"},
	{3181, "Function Code"},
	{3182, "HSN Member Number"},
	{3183, "Mother's Maiden Name"},
	{3184, "Special Process Code"},
	{3185, "ID Number"},
	{3186, "ID State"},
	{3187, "ID Expire"},
	{3188, "Has MC"},
	{3189, "Has Visa"},
	{3190, "Has Amex"},
	{3191, "Has Sears/Disc"},
	{3192, "Has Dept Store"},
	{3193, "Credit Type/Joint Auth Code"},
	{3194, "ID Verified"},
	{3195, "Application Signed"},
	{3196, "Joint Signed"},
	{3197, "Store Employee"},
	{3198, "Promo Code"},
	{3199, "Promo Tracking"},
	{3200, "Contract ID"},
	{3201, "Contract Sequence"},
	{3202, "External Override Flag"},
	{3203, "Que Indicator Flag"},
	{3204, "Register Time"},
	{3205, "Comment"},
	{3206, "Image Document"},
	{3207, "Foreign Language Indicator"},
	{3208, "Client Department Number"},
	{3220, "Spouse First Name"},
	{3221, "Spouse Middle Initial"},
	{3222, "Spouse Last Name"},
	{3223, "Spouse Address 1 "},
	{3224, "Spouse Address 2 "},
	{3225, "Spouse City "},
	{3226, "Spouse State "},
	{3227, "Spouse Zip Code "},
	{3300, "Application Status"},
	{3301, "Application Error Codes"},
	{3302, "Application State Code"},
	{3303, "Credit Limit"},
	{3304, "Approved Credit Type"},
	{3305, "Approved Account Number"},
	{3306, "Approved Credit Line"},
	{3307, "Monthly Interest Rate"},
	{3308, "Annual Percentage Rate"},
	{3309, "Receipt Logo"},
	{3310, "Receipt Header"},
	{3311, "Receipt Footer"},
	{3312, "Receipt Promotional Message"},
	{3313, "Account Number Digits"},
	{3314, "External Reference Number"},
	{3315, "External Reference Phone #"},
	{3316, "External Credit Line"},
	{3317, "Authorization Code"},
	{3318, "Unused"},
	{3319, "Unused"},
	{3320, "Credit Score"},
	{3350, "Down Payment"},
	{3351, "Unpaid Balance"},
	{3352, "Annual Percentage Rate"},
	{3353, "Contract Date"},
	{3354, "Number of Payments"},
	{3355, "First Payment Date"},
	{3356, "First Payment Amount"},
	{3357, "Second Payment Date"},
	{3358, "Second Payment Amount"},
	{3359, "Same as Cash"},
	{3360, "Deferred Time"},
	{3361, "Collateral Description"},
	{3362, "Life Insurance Indicator"},
	{3363, "Accident/Health Insurance Indicator"},
	{3364, "PPI Coverage Term"},
	{3365, "PPI Coverage Amount"},
	{3366, "PPI State Code"},
	{3367, "Foreign Insurance Premium"},
	{3368, "Filing Fee"},
	{3369, "Non-filing Fee"},
	{3370, "Fee Code"},
	{4000, "Processor Code"},
	{4001, "Merchant Reference Number"},
	{4002, "Host Reference Number"},
	{4003, "Case Number"},
	{4004, "Ref Number (ARN)"},
	{4005, "Control Number"},
	{4006, "Batch Number"},
	{4007, "Orig Amount"},
	{4008, "Disputed Amount"},
	{4009, "Credit Flag"},
	{4010, "Fees"},
	{4011, "Adjustment"},
	{4012, "Card Number"},
	{4013, "Card Kind"},
	{4014, "Tran Date"},
	{4015, "Batch Date"},
	{4016, "Retrieval Date"},
	{4017, "Doc Date"},
	{4018, "Response Date"},
	{4019, "Merchant ID"},
	{4020, "Merchant Name"},
	{4021, "Merchant City"},
	{4022, "Merchant State"},
	{4023, "Status"},
	{4024, "Status Description"},
	{4025, "Response Text 1"},
	{4026, "Response Text 2"},
	{4027, "Response Text 3"},
	{4028, "Reason Code"},
	{4029, "Reason Description"},
	{4030, "Issuer Bin ICA"},
	{4031, "Acquiring Bin ICA"},
	{4032, "Charge Off Date"},
	{4033, "Recredit Date"},
	{4034, "Complete Date"},
	{4035, "Post To Merchant Date"},
	{4036, "Response Due Flag"},
	{4037, "Merchant Accepted Flag"},
	{4038, "Merchant Accepted Date"},
	{4039, "Last Updated"},
	{4040, "Case Type"},
	{4041, "Sequence ID"},
	{4100, "Processsor Specific Value 01"},
	{4101, "Processsor Specific Label 01"},
	{4150, "Processsor Specific Value 25"},
	{4151, "Processsor Specific Label 25"},
	{4200, "Processsor Specific Value 50"},
	{4201, "Processsor Specific Label 50"},
	{4250, "Processsor Specific Value 75"},
	{4251, "Processsor Specific Label 75"},
	{4298, "Processsor Specific Label 99"},
	{4299, "Processsor Specific Value 99"},
	{4444, "Response Record Format"},
	{5000, "Signature Capture Blob"},
	{5001, "DevTrans Non Financial Data"},
	{7002, "Message Request Time"},
	{7003, "Message Response Time"},
	{7004, "Third Party Processor Request Time."},
	{7005, "Third Party Processor Response Time."},
	{7006, "Configuration Version"},
	{7007, "Transaction Link Identifier"},
	{7008, "SDC Terminal ID"},
	{7009, "Primary Account Number"},
	{7010, "Currency Code"},
	{8000, "Gateway - remote prog to run"},
	{8001, "PbAdmin Supplemental Info"},
	{8002, "Location Name or Source IP Address"},
	{8003, "Connect to address"},
	{8004, "Group Name"},
	{8005, "Application  Versions"},
	{8013, "System Date"},
	{8014, "System Time"},
	{9994, "Unique_ID for debug logs"},
	{9995, "Timestamps"},
	{9996, "Request Packet"},
	{9997, "Status Message IPC Handle"},
	{9998, "Received Packet"},
	{9999, "Transmit Packet"},
	{0, NULL}
};


static char *
stristr(const char *s0, const char *s1)
{
	char *str[2];
	const char *cp;
	char *ptr;



	{
		if ((str[0] = (char *)alloca(strlen(s0) + 1)) == NULL)
			return NULL;
		if ((str[1] = (char *)alloca(strlen(s1) + 1)) == NULL)
			return NULL;
	}

	for (ptr = str[0], cp = s0; *cp;)
	{
		*ptr++ = tolower(*cp);
		++cp;
	}
	*ptr = '\0';

	for (ptr = str[1], cp = s1; *cp;)
	{
		*ptr++ = tolower(*cp);
		++cp;
	}
	*ptr = '\0';

	if ((ptr = strstr(str[0], str[1])) != NULL)
		ptr = (char *)&s0[(ptr - str[0])];

	return ptr;
}


static void
SplitFile(FILE *f)
{
	static char bfr[1024*1024];
	while ( fgets(bfr, sizeof(bfr), f) != NULL )
	{
		while ( isspace(bfr[0]) )
			strcpy(bfr, &bfr[1]);

		if ( bfr[0] == '#' )
			continue;			// skip comments

		if ( ! isdigit(bfr[0]) )
		{
			if ( bfr[0] == 0x04 )		// eot
				continue;
			if ( bfr[0] == 0x1E )		// eot
				continue;
			fprintf(stderr, "No api field number: %s\n", bfr);
			return;
		}
		int n = atoi(bfr);

		char *data = strchr(bfr, ',');
		if ( data == NULL )
		{
			data = bfr;
		}
		else
		{
			++data;		// skip comma
			while(isspace(*data))
				++data;
		}
		while(data[0] && isspace(data[strlen(data)-1]))
			data[strlen(data)-1] = '\0';

		while ( isspace(*data) )
			strcpy(data, &data[1]);
		Api_t *api = ApiTable;
		char *tag = "Unknown";
		for ( api = ApiTable; api->tag != NULL && api->n != n; ++api);
		if ( api->tag != NULL && api->n == n )
			tag = api->tag;
		printf("%4d %-32.32s %s\n", n, tag, data);
	}
}


static void
Split(char *fname)
{
	FILE	*f = fopen(fname, "r");

	if ( f == NULL )
	{
		fprintf(stderr, "Unable to open %s: ", fname);
		perror("fopen");
	}
	else
	{
		SplitFile(f);
		fclose(f);
	}
}


static void
TellApi(char *arg)
{
	while ( isspace(*arg) )
		++arg;

	int n = -1;
	if ( isdigit(*arg) )
		n = atoi(arg);

	int ii = 0;
	printf("%s finds:\n", arg);
	for ( Api_t *api = ApiTable; api->tag != NULL; ++api )
	{
		if ( n > 0 )
		{
			if ( api->n == n )
			{
				printf("%4d %s\n", api->n, api->tag);
				++ii;
			}
		}
		else
		{
			if ( stristr(api->tag, arg) != NULL )
			{
				printf("%4d %s\n", api->n, api->tag);
				++ii;
			}
		}
	}

	if ( ii <= 0 )
		printf("nothing\n");
}


static void
Usage(char *pname)
{
	fprintf(stderr, "%s  api number or text\n", pname);
	exit(1);
}


int
main(int argc, char *argv[])
{
	int tellapi = (stristr(argv[0], "tellapi") != NULL);

	if ( --argc > 0 )
	{
		for(char *arg; (arg = *++argv) != NULL;)
		{
			if ( tellapi )
				TellApi(arg);
			else
				Split(arg);
		}
	}
	else
	{
		if ( tellapi )
			Usage(argv[0]);
		SplitFile(stdin);
	}

	return 0;
}
